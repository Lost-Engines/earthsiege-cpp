#include "cockpit_displays.h"
#include <algorithm>
#include <utility>

namespace earthsiege {
cockpit_displays::cockpit_displays(resource_store const &resources, cockpit_layout const &layout,
  mission_session &session, pointer_controls &input, random_state &ui_random)
  : random{ui_random}, failure_font{read_font(resources.read("simvol0","dfn/CPRED.DFN"))},
    failure_label{read_interface_strings(resources.read("simvol0","str/STRINGS0.STR")).at(30).at(0).text}, controls{resources,layout}, controller{session.player_id()}, communications{resources},
    status{resources}, target{resources}, scanner{resources},
    navigation{cache,session.scenery()}, missile{resources,cache,session.scenery()} {
  auto const bounds{controls.button_bounds()};
  for(size_t i{};i<buttons.size();++i) buttons[i]=input.add_button(bounds[i]);
  auto const row_bounds{communications.row_bounds(controls.display_bounds())};
  for(size_t i{};i<rows.size();++i) rows[i]=input.add_button(row_bounds[i]);
  enable_controls(input);
}
void cockpit_displays::enable_controls(pointer_controls &input) {
  auto const enabled{mfd_enabled_buttons(controller.state().page)};
  auto const set{[&](size_t i,bool value){if((input.at(i).face!=control_face::disabled)!=value)input.enable(i,value);}};
  for(size_t i{};i<buttons.size();++i) set(buttons[i],enabled[i]);
  for(size_t i{};i<rows.size();++i) set(rows[i],controller.state().page==mfd_page::communications && !choices.choices[i].disabled);
}
void cockpit_displays::transmit(size_t const choice, mission_session &session) {
  choices.selected=choice;
  if(session.transmit_order(static_cast<uint16_t>(choices.command())) && choice>=4)
    choices.choices[choice].alternate=!choices.choices[choice].alternate;
}
bool cockpit_displays::command(uint16_t const code, mission_session &session, pointer_controls &input) {
  if(code>=59 && code<=64) return activate({buttons[code-59],1},session,input);
  if(visible){
    // Original 3a5e7 consumes these page-specific keys even when the
    // corresponding button is disabled. Alt+R also cycles hidden ranges.
    auto const enabled{mfd_enabled_buttons(controller.state().page)};
    constexpr std::array<uint16_t,6> keys{32,0x213,20,45,51,52};
    auto const key{std::ranges::find(keys,code)};
    if(key!=keys.end()){
      auto const button{size_t(key-keys.begin())+7};
      if(enabled[button])activate({buttons[button],1,input.is_double_click()},session,input);
      else if(button==8){controller.button(8,session.player_id(),session.target_selection(),choices.choices);}
      return true;
    }
    if(controller.state().page==mfd_page::communications){
      constexpr std::array<uint16_t,7> names{30,34,35,24,46,18,33};
      constexpr std::array<size_t,7> choices_by_key{0,1,2,3,4,4,5};
      auto const name{std::ranges::find(names,code)};
      if(name!=names.end()){controller.select_choice(choices_by_key[size_t(name-names.begin())]);return true;}
    }
  }
  size_t choice;
  switch(code) {
    case 0x21e: choice=0; break; // Alt+A: attack
    case 0x222: choice=1; break; // Alt+G: ignore
    case 0x223: choice=2; break; // Alt+H: help
    case 0x218: choice=3; break; // Alt+O: form up
    case 0x22e: if(choices.choices[4].alternate) return true; choice=4; break; // Alt+C: scan
    case 0x212: if(!choices.choices[4].alternate) return true; choice=4; break; // Alt+E: EMCON
    case 0x221: choice=5; break; // Alt+F: fire/hold
    default: return false;
  }
  transmit(choice,session);
  if(controller.state().page==mfd_page::communications) controller.select_choice(choice);
  return true;
}
bool cockpit_displays::activate(control_activation const &action, mission_session &session, pointer_controls &input) {
  for(size_t i{};i<buttons.size();++i) if(action.index==buttons[i]) {
    if(!(action.value&1))return true;
    auto const result{controller.button(i,session.player_id(),session.target_selection(),choices.choices)};
    switch(result) {
      case mfd_action::toggle_radar: session.player_command(19); break;
      case mfd_action::cycle_target: session.cycle_target(); break;
      case mfd_action::transmit:
        if(!action.double_click)transmit(choices.selected,session);
        break;
      case mfd_action::none: break;
    }
    sounds.push_back(17);enable_controls(input); return true;
  }
  for(size_t i{};i<rows.size();++i) if(action.index==rows[i]) {
    // Original 3ac04 routes a same-row double click to TRANSMIT, whose
    // 3a46c callback rejects it because the shared double-click latch is set.
    if(!action.double_click || controller.state().choice!=i)controller.select_choice(i);
    if(action.value&1)sounds.push_back(17);
    return true;
  }
  return false;
}
void cockpit_displays::paint(mfd_page const page, mission_session &session, camera_state const &camera, uint32_t const ticks) {
  auto const &bounds{painted_bounds};
  switch(page) {
    case mfd_page::status: status.draw(cache,bounds,session.instrument_damage(controller.state().selected_actor)); break;
    case mfd_page::target: target.draw(cache,bounds,session.instrument_damage(controller.state().selected_actor)); break;
    case mfd_page::communications:
      choices.selected=controller.state().choice; communications.draw(cache,bounds,choices); break;
    case mfd_page::navigation: navigation.draw(bounds,camera.position,camera.angles,{}); break;
    case mfd_page::scanner: scanner.draw(cache,bounds,session.scanner_state(camera.angles.z,ticks,mfd_ranges[controller.state().range_index])); break;
    case mfd_page::missile_camera: {
      std::vector<missile_camera_object> missiles;
      session.ordnance().live().for_each([&](size_t id,live_projectile const &object) {
        if(auto const *instance{std::get_if<projectile_instance>(&object.state)}) {
          auto const &flight{instance->flight()};
          if(flight.kind==projectile_kind::rocket) missiles.push_back({id,vec3<int32_t>{flight.position},vec3<uint16_t>{flight.angles}});
        }
      });
      std::vector<missile_camera_explosion> explosions;
      auto &effects{session.visual_effects()};
      effects.live_explosions().for_each([&](size_t id,live_explosion const &object){explosions.push_back({id,vec3<int32_t>{object.state.position}});});
      std::ranges::reverse(explosions); // The original camera searches oldest-first.
      auto const image{tracker.advance(missiles,explosions)};
      auto const objects{session.visible_objects()}; auto const ground{session.ground_models()};
      missile.draw(bounds,image,session.camera_ready(),session.player_sensors().hud_locked,ticks,{},objects,&effects.lights(),ground);
      break;
    }
  }
}
void cockpit_displays::update(mission_session &session, pointer_controls &input, camera_state const &camera,
  uint32_t const ticks, int const origin_x, int const origin_y) {
  auto bounds{controls.display_bounds()}; bounds.left-=origin_x; bounds.right-=origin_x; bounds.top-=origin_y; bounds.bottom-=origin_y;
  bool const inside{camera.mode==camera_mode::attached};
  bool const shown{inside && bounds.right>=0 && bounds.left<static_cast<int>(screen_width) && bounds.bottom>=0 && bounds.top<static_cast<int>(screen_height)};
  if(shown && (!visible || !initialized || bounds.left!=painted_bounds.left || bounds.top!=painted_bounds.top)) {
    controller.repaint(); initialized=true;
  }
  visible=shown; painted_bounds=bounds;
  auto const &launch{session.latest_player_rocket()};
  if(launch.sequence!=watched_sequence) {watched_sequence=launch.sequence;tracker.watch(launch.slot);}
  auto const severity{instrument_damage_band(session.player_actor().internal_damage_fraction(4))};
  failure.repaint=controller.state().pending_paint;
  if(inside) {
    if(advance_instrument_failure(failure,severity,ticks,random))sounds.push_back(28);
    if(failure.repaint==2)controller.repaint();
  }
  for(auto const &request:controller.advance(ticks,shown,!inside,session.eo_selected(),session.player_id(),session.target_selection(),failure.failed,severity)) {
    if(request.failure) {
      cache.fill(bounds,63);
      if(request.failure>=2) {
        int const x{bounds.left+((bounds.right-bounds.left)>>1)-((failure_font.text_width(failure_label)-1)>>1)};
        int const y{bounds.top+((bounds.bottom-bounds.top+static_cast<int>(failure_font.height))>>1)};
        cache.text(failure_font,failure_label,x,y);
      }
    } else paint(request.page,session,camera,ticks);
  }
  failure.repaint=controller.state().pending_paint;
  enable_controls(input);
}
void cockpit_displays::draw(framebuffer &frame, pointer_controls const &input, int const offset_x, int const offset_y) const {
  std::array<unsigned int,mfd_button_count> faces;
  for(size_t i{};i<faces.size();++i) faces[i]=std::to_underlying(input.at(buttons[i]).face);
  controls.draw(frame,controller.state().page,faces,offset_x,offset_y);
  if(!visible) return;
  auto const &pixels{cache.pixels()};
  for(int y{std::max(0,painted_bounds.top)};y<=std::min(static_cast<int>(screen_height)-1,painted_bounds.bottom);++y)
    for(int x{std::max(0,painted_bounds.left)};x<=std::min(static_cast<int>(screen_width)-1,painted_bounds.right);++x)
      frame.pixel(x,y,pixels[static_cast<size_t>(y)*screen_width+static_cast<size_t>(x)]);
}
std::vector<uint16_t> cockpit_displays::take_sounds() {return std::exchange(sounds,{});}
mfd_state const &cockpit_displays::state() const {return controller.state();}
} // namespace earthsiege
