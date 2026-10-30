#include "mission_view.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/frame_time.h"
#include "simulation/model_math.h"
#include "simulation/tape_input.h"

namespace earthsiege {
namespace {

input_tape demo_tape(resource_store const &resources,unsigned demo){
  if(demo<1||demo>4)throw std::runtime_error{"demo must be between 1 and 4"};
  return read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")));
}
std::unique_ptr<mission_session> demo_start(resource_store const &resources,input_tape const &tape) {
  auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
  auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
  auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
  return std::make_unique<mission_session>(resources, script, roster, random_state{}, counters);
}

} // namespace

mission_view::mission_view(resource_store const &resources, unsigned int const demo)
  : mission_view{resources,demo_tape(resources,demo)} {}
mission_view::mission_view(resource_store const &resources,input_tape const &tape)
  :mission_view{resources,demo_start(resources,tape)} {
  // Playback restores files beside the TAP, while runtime controls still read
  // DATA/PREFS.CFG and DATA/KEYJOY.CFG. The caller supplies installed preferences.
}

mission_view::mission_view(resource_store const &resources, std::unique_ptr<mission_session> mission)
  : session{std::move(mission)}, ui_random{session->interface_random()},
    audio{resources},
    cockpit{read_cockpit(resources, read_resource_names(resources.read("simvol0", "nam/MECHS.NAM")).at(static_cast<size_t>(session->player_chassis())))},
    reactor{resources, cockpit.layout},
    throttle{resources, cockpit.layout},
    shields{resources, cockpit.layout},
    weapons{resources, cockpit.layout},
    navigation{resources, cockpit.layout},
    waypoints{resources, cockpit.layout},
    clock{resources, cockpit.layout},
    lamps{resources, "simvol0", "ttm/" + read_resource_names(resources.read("simvol0", "nam/MECHS.NAM")).at(static_cast<size_t>(session->player_chassis())) + ".TTM"},
    display_colours{session->scenery().colours()},
    impact_colours{read_palette(resources.read("simvol0","dpl/IMPACT1.DPL"))},
    environment{resources,session->scenery().environment()},
    renderer{frame, session->scenery(), read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))},
    camera{session->make_camera(camera_mode::attached)} {
  // The shipped file is an external input policy, separate from PREFS.CFG.
  for(auto const &path:{resources.root()/"DATA"/"KEYJOY.CFG",resources.root()/"data"/"keyjoy.cfg"}){
    if(!std::filesystem::exists(path))continue;
    auto raw{read_file(path)};auto options{read_keyjoy(std::string_view{reinterpret_cast<char const *>(raw.data()),raw.size()})};
    if(options.invalid_token)throw std::runtime_error{"Unrecognized KEYJOY.CFG token in "+path.string()};
    routing.tilt_reverse=options.tilt_reverse;routing.backturn_reverse=options.backturn_reverse;routing.missile_reverse=options.missile_reverse;break;
  }
  apply_cockpit_colours(display_colours, read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL")), static_cast<unsigned int>(session->player_chassis()));
  update_camera(camera, session->view_target());
  environment.initialize_palette(display_colours,camera.angles.x);
  session->on_cockpit_impact([this]{impact.hit(interface_ticks,impact_view(),ui_random);});
  audio.play(19);
  clock.refresh();
  lamps.advance(0);
  instruments.add_slider(throttle.slider_layout(), 0);
  for(auto const &bounds : shields.button_bounds()) instruments.add_button(bounds);
  session->initialise_weapon_instruments();
  auto const rows{session->player_weapon_controls().displays()};
  for(size_t i{}; i < rows.size(); ++i) {
    auto const &row{rows[i]};
    weapon_control_binding binding{i, instruments.add_button(weapons.button_bounds(row.control_number, row.kind)), {}, row.state.requested_charge};
    if(row.kind == weapon_instrument_kind::energy) binding.slider = instruments.add_slider(weapons.charge_slider(row.control_number), row.state.requested_charge);
    weapon_controls.push_back(binding);
  }
  auto const groups{weapons.group_bounds()};
  for(size_t i{}; i < groups.size(); ++i) weapon_groups[i] = instruments.add_button(groups[i]);
  radio=std::make_unique<cockpit_radio>(resources,cockpit.layout,*session,ui_random);
  displays=std::make_unique<cockpit_displays>(resources,cockpit.layout,*session,instruments,ui_random);
  hdd=std::make_unique<cockpit_hdd>(resources,cockpit.layout,*session,instruments,ui_random);
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

cockpit_impact_view mission_view::impact_view()const{
 return {.mode{camera.mode==camera_mode::attached?static_cast<unsigned>(panel):4},
         .origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin}};
}
std::array<int32_t,2> mission_view::instrument_origin()const{
 auto origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin};
 if(camera.mode==camera_mode::attached)origin[1]+=impact.snapshot().displacement;
 return origin;
}

void mission_view::apply_preferences(preferences const &prefs){
  auto const &p{prefs.current()};
  if(audio.preferences(p[0]!=0,p[1]!=0,p[2]!=0))audio.update_music(session->music_observations(),ui_random);
  radio->captions(p[2]!=1);
  environment_detail(p[3]!=0,p[4],p[5]);
  renderer.shape_detail(p[6]);
}
void mission_view::configure_input(preferences const &prefs,std::array<joystick_calibration,2> const &values){
  apply_preferences(prefs);
  auto const &p{prefs.current()};routing.model=p[7];routing.mode=p[8];routing.pedals=p[9]!=0;routing.wcs=p[10]!=0;mouse_enabled=p[11]!=0;joystick_mode_enabled=p[8]!=0;calibrations=values;
  clear_input();
}
void mission_view::hat_view(std::array<uint8_t,4> const &hat){
  if(camera.mode!=camera_mode::attached)return;
  transitions.request(cockpit_hat_request(static_cast<unsigned>(panel),hat));
}
void mission_view::key(held_key const key, bool const pressed, bool const alt) {
  keyboard.event(key, pressed, alt);
}

void mission_view::command(uint16_t const code) { commands.push_back(code); }
void mission_view::mouse_position(int const x, int const y) { if(!mouse_enabled)return; pointer.mouse = {x,y}; pointer_events.push_back({pointer,{}}); }
void mission_view::mouse_button(bool const right, bool const pressed) {
  if(!mouse_enabled)return;
  (right ? pointer.right : pointer.left) = pressed;
  pointer_events.push_back({pointer,{}});
}

void mission_view::consume_pointer_events() {
  auto const origin{instrument_origin()};
  instruments.window_bounds({origin[0], origin[1], origin[0] + 319, origin[1] + 199});
  if(camera.mode == camera_mode::attached) for(auto const &queued : pointer_events) {
    auto const &event{queued.input};
    hdd->pointer(event.mouse.x+origin[0],event.mouse.y+origin[1]);
    if(auto const action{instruments.event(event.mouse.x + origin[0], event.mouse.y + origin[1], event.left, event.right,queued.timestamp.value_or(interface_ticks*2))}) {
      if(hdd->activate(*action,*session,instruments,mfd_ranges[displays->state().range_index]))continue;
      if(displays->activate(*action,*session,instruments)) continue;
      if(action->index == 0) requested_throttle = static_cast<int16_t>(throttle.slider_layout().vertical ? -action->value : action->value);
      else if(action->index < 3) { if(action->value & 1) shield_to_front = action->index == 1; }
      else {
        if(action->value & 1) for(size_t i{}; i < weapon_groups.size(); ++i)
          if(action->index == weapon_groups[i]) session->weapon_group_control(static_cast<unsigned int>(i));
        for(auto &binding : weapon_controls) {
          auto const &row{session->player_weapon_controls().displays()[binding.row]};
          if(action->index == binding.button) session->weapon_control(row.control_number, (action->value & 2) != 0);
          else if(binding.slider == action->index) {
            binding.requested_charge = std::clamp(action->value, 0, 1024);
            session->weapon_charge_request(row.control_number, binding.requested_charge);
          }
        }
      }
    }
  }
  pointer_events.clear();
}

void mission_view::clear_input() {
  keyboard.clear();
  hardware={};device_throttle=0;routing.switch_held=false;
  pointer.left = pointer.right = false;
  pointer_events.clear();
  instruments.reset_input();
  shield_to_front.reset();
  commands.clear();
  session->clear_player_input();
  camera.angular_rates = vec3<int16_t>{0, 0, 0};
  camera.movement = 0;
}

void mission_view::toggle_pause() {
  paused = !paused;
  clear_input();
}

void mission_view::change_camera(camera_mode const mode) { transitions.reset(mode==camera_mode::attached?static_cast<int>(panel):4);set_camera(mode,impact_view().mode); }
void mission_view::set_camera(camera_mode const mode,unsigned previous,bool reset_input) {
  if(mode == camera_mode::free) camera.mode = mode;                             // detach at the current position and orientation
  else camera = session->make_camera(mode);
  impact.change_view(previous,impact_view());
  if(reset_input){clear_input();external_view_requested=external_view_active=mode!=camera_mode::attached;}
  update_camera(camera, session->view_target());
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

void mission_view::change_cockpit_view(cockpit_view const view) {
  transitions.reset(static_cast<int>(view));apply_cockpit_view(view,true);
}
void mission_view::apply_cockpit_view(cockpit_view const view,bool reset_input) {
  if(panel != view) clock.refresh();
  auto const previous{impact_view().mode};
  panel = view;
  set_camera(camera_mode::attached,previous,reset_input);
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
}

void mission_view::present_view(){
  if(!present_pending||options_camera)return;
  present_pending=false;
  auto before{transitions.snapshot().view};transitions.present();
  auto after{transitions.snapshot().view};
  if(before==after)return;
  if(after==4)set_camera(camera_mode::orbit,impact_view().mode,false);
  else apply_cockpit_view(static_cast<cockpit_view>(after),false);
}

void mission_view::advance_time(unsigned milliseconds){
  auto const before{elapsed_milliseconds*120/1000};elapsed_milliseconds+=milliseconds;
  auto const ticks{static_cast<unsigned>(elapsed_milliseconds*120/1000-before)};
  pacing.advance(ticks);audio.advance(ticks);
}
bool mission_view::step(bool const control, bool const zoom) {
  auto const ratio{pacing.take()};
  if(!ratio)return false;
  if(paused) { pointer_events.clear(); return false; }
  prepare_frame(pacing.state().slow);
  routing.external=control||(external_view_active&&external_camera_control);routing.missile=session->player_weapon_controls().manual_guidance();
  auto const keys{keyboard.sample((routing.mode==1&&!routing.external)||routing.missile)};
  joystick_sample keyboard_packet{{keys.movement.x,keys.movement.y,keys.aiming.x,keys.aiming.y},{uint8_t(keys.fire),uint8_t(keys.enter),0,0},{}};
  joystick_sample device;
  auto const profiles{joystick_profiles(routing.model,routing.pedals)};
  for(size_t i{};i<2;++i)if(hardware[i].present)
    combine_joystick(device,sample_joystick(profiles[i],calibrations[i],hardware[i].axes,hardware[i].buttons));
  auto const packet{route_devices(routing,keyboard_packet,device,device_throttle)};
  player_input input{pointer};
  input.movement={packet.axes[0],packet.axes[1]};input.aiming={packet.axes[2],packet.axes[3]};
  input.camera_source=packet.aiming_camera?player_axis_pair::aiming:player_axis_pair::movement;
  input.fire=packet.buttons[0]||(control&&zoom);input.enter=packet.buttons[2];input.released=packet.buttons[3];
  input.release_action=static_cast<control_release_action>(routing.mode);
  if(!commands.empty()){input.command=commands.front();commands.pop_front();}
  return step_input(input,packet.hat,*ratio,routing.external);
}
void mission_view::prepare_frame(uint32_t now){
  audio.engine(camera.mode!=camera_mode::attached);
  audio.update_music(session->music_observations(),ui_random);
  clock.update(now);
  if(now!=interface_ticks)lamps.advance(static_cast<int16_t>(now-interface_ticks));
  interface_ticks=now;
  // Original 4cb14 consumes the mouse queue before world simulation.
  consume_pointer_events();
}
bool mission_view::step_recorded(tape_frame const &record,uint32_t slow_clock,bool suppress_camera,std::function<bool(uint16_t)> const &modal){
  for(auto const &event:record.mouse){
    player_input mouse;mouse.mouse=event.position;mouse.left=(event.kind&1)!=0;mouse.right=(event.kind&2)!=0;
    pointer_events.push_back({mouse,event.timestamp});
  }
  prepare_frame(slow_clock);
  routing.external=external_view_active&&external_camera_control;routing.missile=session->player_weapon_controls().manual_guidance();
  auto const packet{route_tape_frame(routing,record,suppress_camera,device_throttle)};
  player_input input;
  input.command=record.command;
  input.movement={packet.axes[0],packet.axes[1]};input.aiming={packet.axes[2],packet.axes[3]};
  input.camera_source=packet.aiming_camera?player_axis_pair::aiming:player_axis_pair::movement;
  input.fire=packet.buttons[0];input.enter=packet.buttons[2];input.released=packet.buttons[3];
  input.release_action=static_cast<control_release_action>(routing.mode);
  return step_input(input,packet.hat,static_cast<frame_ratio>(record.delta),routing.external,modal);
}
bool mission_view::step_input(player_input initial_input,std::array<uint8_t,4> const &hat,frame_ratio ratio,bool control,std::function<bool(uint16_t)> const &modal){
  auto dispatch=[&](player_input &input){
  if(input.command && modal && modal(input.command))input.command=0;
  if(input.command) {
    if(input.command==47){external_view_requested=!external_view_active;transitions.request(external_view_requested?2:3);input.command=0;}
    if((input.command==15||input.command==28)&&camera.mode!=camera_mode::attached){external_camera_control=!external_camera_control;input.command=0;}
    // 38034 routes TRACK through the widget. The bank reads the pending
    // flag during the later instrument exchange, after this frame's aiming.
    if(input.command==0x214&&camera.mode==camera_mode::attached&&panel!=cockpit_view::lower){session->weapon_group_control(4);input.command=0;}
    if(input.command>=59 && input.command<=64 && camera.mode==camera_mode::attached && panel!=cockpit_view::forward)
      transitions.request(panel==cockpit_view::lower?1:6);
    if(input.command==65 || input.command==66)transitions.request(0);
    if(input.command==82){if(joystick_mode_enabled)routing.mode=static_cast<uint8_t>(3-routing.mode);input.command=0;}
    if(input.command==67){hat_view({0,0,0,1});input.command=0;}
    if(input.command==68){hat_view({0,0,1,0});input.command=0;}
    if(input.command==1&&camera.mode==camera_mode::attached&&panel!=cockpit_view::forward){transitions.request(panel==cockpit_view::lower?1:6);input.command=0;}
    if(hdd->command(input.command,*session,instruments,mfd_ranges[displays->state().range_index]))input.command=0;
    else if(displays->command(input.command,*session,instruments))input.command=0;
  }
  hat_view(hat);
  input.release_action=static_cast<control_release_action>(routing.mode);
  };
  auto const actions{session->step_player(initial_input, ratio, control, cockpit.layout.bounds(0x450),dispatch)};
  steer_camera(camera, actions.zoom, actions.camera.y, actions.camera.x);
  auto const throttle_request{session->exchange_player_instruments(requested_throttle, shield_to_front)};
  shield_to_front.reset();
  if(requested_throttle != throttle_request) {
    requested_throttle = throttle_request;
    instruments.set_value(0, requested_throttle);
  }
  session->refresh_target_instruments(camera.mode==camera_mode::attached&&panel==cockpit_view::forward,true);
  session->update_player_interface();
  session->exchange_weapon_instruments(ratio);
  for(auto &binding : weapon_controls) {
    auto const &state{session->player_weapon_controls().displays()[binding.row].state};
    auto const update_enabled{[&](size_t const index){
      if((instruments.at(index).face == control_face::disabled) != state.disabled) instruments.enable(index, !state.disabled);
    }};
    update_enabled(binding.button);
    if(binding.slider) {
      update_enabled(*binding.slider);
      // The original setter leaves a live drag alone unless the requested
      // value changed; keyboard changes override the drag on that exchange.
      if(binding.requested_charge != state.requested_charge) {
        binding.requested_charge = state.requested_charge;
        instruments.set_value(*binding.slider, state.requested_charge);
      }
    }
  }
  update_camera(camera, session->view_target());
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
  impact.update(interface_ticks,impact_view(),ui_random);
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
  for(auto const &request:session->take_sounds())audio.dispatch(request,player.body().movement().position);
  for(auto const id:displays->take_sounds())audio.play(id);
  for(auto const id:radio->take_sounds())audio.play(id);
  for(auto const id:hdd->take_sounds())audio.play(id);
  for(auto const &path:radio->take_speech())audio.speech(path);
  audio.flush_positions(player.body().movement().position);
  // Camera ownership changes in 5894c after this frame's player controls,
  // before the later 22904/226f0 presentation can prepare another view.
  if(transitions.snapshot().prepared)external_view_active=external_view_requested;
  present_pending=true;
  return true;
}

void mission_view::begin_options(){
  if(options_camera)return;
  clear_input();options_camera=camera;
  camera=session->make_camera(camera_mode::orbit);camera.orbit_distance=0;
  update_camera(camera,session->view_target());
}
bool mission_view::step_options(){
  if(!options_camera||!pacing.take())return false;
  // Original 577f8: fixed camera-turn request, no world simulation.
  steer_camera(camera,false,0,-64);update_camera(camera,session->view_target());return true;
}
void mission_view::end_options(){
  if(!options_camera)return;
  camera=*options_camera;options_camera.reset();clear_input();
}
void mission_view::draw() {
  frame.clear();
  bool const inside{camera.mode == camera_mode::attached};
  auto const &window{cockpit.windows[static_cast<size_t>(panel)]};
  auto const shift{inside?impact.snapshot().displacement:0};
  auto mask{window.mask};
  if(shift)mask.translate(0,-shift);
  auto &colours{impact.snapshot().impact_palette?impact_colours:display_colours};
  model_view view;
  if(options_camera){view.bounds={0,0,319,110};view.projection.centre={160,50};}
  if(inside) {
    cockpit.draw(frame, panel,shift);
    auto const centre{window.projection_centre()};
    view.projection.centre = {centre[0], centre[1]};
    view.bounds = window.bounds;
  }
  {
    scoped_framebuffer_clip clip{frame, inside ? &mask : nullptr};
    environment.draw(frame,camera.position,camera.angles,view.projection,view.bounds,ui_random,colours,detail,{0,0},impact.snapshot().flash);
    auto const objects{session->visible_objects(!inside)};
    auto &effects{session->visual_effects()};
    auto const ground{effects.ground_flashes()};
    renderer.draw_objects(camera.position, camera.angles, {}, false, objects, &effects.lights(), ground, view);
  }
  session->refresh_target_instruments(inside&&panel==cockpit_view::forward);
  if(inside) {
    auto const &player{session->player_actor()};
    auto const &movement{player.body().movement()};
    auto const &profile{player.profile().movement};
    auto const origin{instrument_origin()};
    int const x{-origin[0]}, y{-origin[1]};
    lamps.draw(frame, x, y);
    clock.draw(frame, x, y);
    reactor.draw(frame, static_cast<int32_t>(player.power().reserve) * 1024 / herc_power_capacity, x, y);
    int const speed_capacity{movement.gait_rate < 0 ? -profile.reverse_rate : profile.forward_rate};
    instruments.painted(0);
    throttle.draw_at(frame, static_cast<int32_t>(movement.gait_rate) * 1024 / speed_capacity, instruments.at(0).painted_thumb, x, y);
    auto const &shield{player.power().shields};
    update_shield_colours(colours, static_cast<int32_t>(shield.front) * 1024 / shield.capacity,
                         static_cast<int32_t>(shield.rear) * 1024 / shield.capacity);
    shields.draw(frame, shield.balance, {std::to_underlying(instruments.at(1).face), std::to_underlying(instruments.at(2).face)}, x, y);
    for(auto const &binding : weapon_controls) {
      auto const &row{session->player_weapon_controls().displays()[binding.row]};
      std::optional<int> boundary;
      if(binding.slider) { instruments.painted(*binding.slider); boundary = instruments.at(*binding.slider).boundary; }
      weapons.draw(frame, row.control_number, row.kind, row.name, row.state, std::to_underlying(instruments.at(binding.button).face), boundary, x, y);
    }
    std::array<unsigned int, 5> faces;
    for(size_t i{}; i < faces.size(); ++i) faces[i] = std::to_underlying(instruments.at(weapon_groups[i]).face);
    weapons.draw_groups(frame, session->player_weapon_controls().current_group(), faces, x, y);
    navigation.draw(frame, x, y, session->player_weapon_controls().auto_tracking());
    displays->draw(frame,instruments,x,y);
    hdd->draw(frame,instruments,x,y);
    if(auto const marker{session->target_marker()}) {
      auto camera_transform{rotation_transform(camera.angles)};
      camera_transform.translation = camera.position;
      if(auto const brackets{target_brackets(*marker, inverse_transform(camera_transform), view.projection)}) {
        scoped_framebuffer_clip target_clip{frame, &mask};
        draw_target_brackets(frame, *brackets, marker->locked, marker->side);
      }
    }
    waypoints.draw(frame, player.body().movement().position, player.body().movement().angles.z, navigation.display_heading(), session->next_waypoint(), x, y);
  }
  if(!options_camera)radio->draw(frame);
  present_view();
}

std::optional<vec2<int32_t>> mission_view::take_pointer_warp(){return hdd->take_warp();}
std::vector<int16_t> mission_view::take_audio(){return audio.take_output();}
framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return impact.snapshot().impact_palette?impact_colours:display_colours; }

} // namespace earthsiege
