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
#include "simulation/fixed_math.h"
#include "simulation/tape_input.h"

namespace earthsiege {
namespace {

input_tape demo_tape(resource_store const &resources,unsigned demo){
  if(demo<1||demo>4)throw std::runtime_error{"demo must be between 1 and 4"};
  return read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")), resources.edition());
}
std::unique_ptr<mission_session> demo_start(resource_store const &resources,input_tape const &tape) {
  auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
  auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)],tape.edition)};
  auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
  return std::make_unique<mission_session>(resources, script, roster, random_state{}, counters);
}

} // namespace

mission_view::mission_view(resource_store const &resources, unsigned int const demo,music_settings const &music)
  : mission_view{resources,demo_tape(resources,demo),music} {}
mission_view::mission_view(resource_store const &resources,input_tape const &tape,music_settings const &music)
  :mission_view{resources,demo_start(resources,tape),music} {
  // Playback restores files beside the TAP, while runtime controls still read
  // DATA/PREFS.CFG and DATA/KEYJOY.CFG. The caller supplies installed preferences.
}

mission_view::mission_view(resource_store const &resources, std::unique_ptr<mission_session> mission,music_settings const &music)
  : session{std::move(mission)}, ui_random{session->interface_random()},
    audio{resources,music},
    cockpit{read_cockpit(resources, read_resource_names(resources.read("simvol0", "nam/MECHS.NAM")).at(static_cast<size_t>(session->player_chassis())))},
    reactor{resources, cockpit.layout},
    throttle{resources, cockpit.layout},
    shields{resources, cockpit.layout},
    weapons{resources, cockpit.layout},
    navigation{resources, cockpit.layout},
    waypoints{resources, cockpit.layout},
    clock{resources, cockpit.layout},
    external_artwork{resources},
    lamps{resources, "simvol0", "ttm/" + read_resource_names(resources.read("simvol0", "nam/MECHS.NAM")).at(static_cast<size_t>(session->player_chassis())) + ".TTM"},
    display_colours{session->scenery().colours()},
    impact_colours{read_palette(resources.read("simvol0","dpl/IMPACT1.DPL"))},
    lift_artwork{read_bitmaps(resources.read("simvol0","dba/INTRO.DBA")).at(0)},
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
  apply_cockpit_colours(display_colours, read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL")), static_cast<unsigned int>(session->player_chassis()), resources.edition());
  update_camera(camera, session->view_target());
  environment.initialize_palette(display_colours,camera.angles.x);
  display_device.present(display_colours);
  session->on_cockpit_impact([this]{impact.hit(interface_ticks,impact_view(),ui_random);});
  // Original startup fills the lower/forward artwork on both page pairs.
  for(int x:{0,320})for(auto view:{cockpit_view::lower,cockpit_view::forward}){
    framebuffer artwork;artwork.clear(63);cockpit.draw(artwork,view);
    surface.store(artwork,x,cockpit.windows[static_cast<size_t>(view)].instrument_origin[1]);
  }
  // Original data byte 94b56 starts lit; both initial VGA pages receive
  // a full clock paint before the first timed update.
  clock.refresh();clock.refresh();clock.initialize_pages();
  lamps.advance(0);
  instruments.add_slider(throttle.slider_layout(), 0);
  for(auto const &bounds : shields.button_bounds()) instruments.add_button(bounds);
  session->initialise_weapon_instruments();
  exchange_panel_values();
  auto const rows{session->player_weapon_controls().displays()};
  painted_weapon_rows.assign(rows.begin(),rows.end());
  for(size_t i{}; i < rows.size(); ++i) {
    auto const &row{rows[i]};
    weapon_control_binding binding{i, instruments.add_button(weapons.button_bounds(row.control_number, row.kind)), {}, row.state.requested_charge};
    if(row.kind == weapon_instrument_kind::energy) binding.slider = instruments.add_slider(weapons.charge_slider(row.control_number), row.state.requested_charge);
    weapon_controls.push_back(binding);
  }
  auto const groups{weapons.group_bounds()};
  for(size_t i{}; i < groups.size(); ++i) weapon_groups[i] = instruments.add_button(groups[i]);
  radio=std::make_unique<cockpit_radio>(resources,cockpit.layout,*session,ui_random);
  weapons.painted_region=[this](rectangle const &bounds){radio->overpaint(bounds);};
  session->sound_output([this](sound_request const &request){audio.dispatch(request,camera.position);});
  radio->sound_output([this](uint16_t id,bool idle_only){if(!idle_only||!audio.playing(id))audio.play(id);});
  radio->speech_output([this](std::string_view path){return audio.speech(path);});
  session->on_radio([this](radio_event const &event){radio->submit(event,interface_ticks);});
  displays=std::make_unique<cockpit_displays>(resources,cockpit.layout,*session,instruments,ui_random);
  hdd=std::make_unique<cockpit_hdd>(resources,cockpit.layout,*session,instruments,ui_random);
  for(size_t i{};i<edge_controls.size();++i)edge_controls[i]=instruments.add_button(cockpit_edge_bounds(0)[i]);
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1],transitions.snapshot().prepared&&transitions.snapshot().pending==3);
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

cockpit_impact_view mission_view::impact_view()const{
 return {.mode{camera.mode==camera_mode::attached?static_cast<unsigned>(panel):4},
         .origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin}};
}
cockpit_view mission_view::painted_panel()const{
 auto const &state{transitions.snapshot()};
 if(state.prepared)switch(state.pending){
  case 0:return cockpit_view::lower;
  case 1:return cockpit_view::forward;
  case 3:return static_cast<cockpit_view>(state.saved_view);
  case 4:return cockpit_view::right;
  case 5:return cockpit_view::left;
  case 6:return cockpit_view::forward;
  default:break;
 }
 return panel;
}
std::array<int32_t,2> mission_view::instrument_origin()const{
 auto origin{cockpit.windows[static_cast<size_t>(painted_panel())].instrument_origin};
 if(camera.mode==camera_mode::attached)origin[1]+=startup?startup->displacement():impact.snapshot().displacement;
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
  auto edges{cockpit_edge_bounds(static_cast<int>(painted_panel()))};
  for(size_t i{};i<edges.size();++i){
    auto b{edges[i]};b.left+=origin[0];b.right+=origin[0];b.top+=origin[1];b.bottom+=origin[1];
    instruments.bounds(edge_controls[i],b);
  }
  if(camera.mode == camera_mode::attached) for(auto const &queued : pointer_events) {
    auto const &event{queued.input};
    hdd->pointer(event.mouse.x+origin[0],event.mouse.y+origin[1]);
    if(auto const action{instruments.event(event.mouse.x + origin[0], event.mouse.y + origin[1], event.left, event.right,queued.timestamp.value_or(interface_ticks*2))}) {
      if(auto const edge{std::ranges::find(edge_controls,action->index)};edge!=edge_controls.end()){
        transitions.request(cockpit_edge_request(static_cast<int>(panel),static_cast<size_t>(edge-edge_controls.begin())));continue;
      }
      if(hdd->activate(*action,*session,instruments,mfd_ranges[displays->state().range_index])){flush_interface_audio();continue;}
      if(displays->activate(*action,*session,instruments)){flush_interface_audio();continue;}
      if(action->index == 0) requested_throttle = static_cast<int16_t>(throttle.slider_layout().vertical ? -action->value : action->value);
      else if(action->index < 3) { if(action->value & 1){shield_to_front = action->index == 1;audio.play(17);} }
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
      if(action->index>=3&&!instruments.at(action->index).slider&&(action->value&1))audio.play(17);
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
  // 5894c/58a44 retain the target camera across cockpit and external views.
  camera.mode = mode;
  impact.change_view(previous,impact_view());
  if(reset_input){clear_input();external_view_requested=external_view_active=mode!=camera_mode::attached;}
  if(reset_input)update_camera(camera, session->view_target());
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

void mission_view::change_cockpit_view(cockpit_view const view) {
  transitions.reset(static_cast<int>(view));apply_cockpit_view(view,true);
}
void mission_view::apply_cockpit_view(cockpit_view const view,bool reset_input) {
  if(panel != view){
    auto const origin{cockpit.windows[static_cast<size_t>(view)].instrument_origin};
    clock.refresh_retained(transitions.snapshot().other_page/2,clock.visible_at(origin[0],origin[1]));
  }
  auto const previous{impact_view().mode};
  panel = view;
  if(reset_input)set_camera(camera_mode::attached,previous,true);
  else impact.change_view(previous,impact_view());
  // A completed scroll changes view ownership after the frame update.
  // Widgets advance on the next frame, not again while presenting the old one.
  if(!reset_input)return;
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1],transitions.snapshot().prepared&&transitions.snapshot().pending==3);
}

void mission_view::present_view(){
  if(!present_pending)return;
  present_pending=false;
  auto const state{transitions.snapshot()};
  if(state.prepared){
    for(auto const &position:cockpit_pan_path(state.pending,state.view,cockpit.windows)){
      auto const page{state.pending<2?state.other_page:state.draw_page};
      framebuffer image;surface.sample(image,page*160+position.x,position.y);
      pan_frames.push_back(std::move(image));
    }
  }
  if(state.prepared&&state.pending==3){
    surface.restore_from_external(state.other_page*160,state.draw_page*160);
    auto const view{static_cast<size_t>(state.saved_view)};
    surface.sample(frame,state.other_page*160,cockpit.windows[view].instrument_origin[1]);
  }
  auto before{state.view};transitions.present();
  auto const after_state{transitions.snapshot()};
  auto after{after_state.view};
  // A prepared cockpit page clears the radio area. Full shield repaints below
  // schedule their palette upload for the following presentation.
  if(!state.prepared&&after_state.prepared&&after_state.pending!=2){
    radio->repaint(); // 2fa00 clears the message rectangle even with no text.
    // Vertical pans reuse the existing pages. Only horizontal preparations
    // and a return from external view invoke the full shield painter.
    if(after_state.pending>=3){
      auto const page{after_state.other_page};
      surface.prepare_artwork(page*160);
      throttle.repaint_lamps();
      weapons.repaint();
      displayed_shields=painted_shields;
      update_shield_colours(display_colours,(*displayed_shields)[0],(*displayed_shields)[1]);
      display_device.changed(80,6);
    }
  }
  if(before==after)return;
  if(after!=4)apply_cockpit_view(static_cast<cockpit_view>(after),false);
}

void mission_view::finish_launch(){
  if(launch_started)return;
  launch_started=true;audio.play(19);
}
void mission_view::begin_startup(){
  if(startup||launch_started)return;
  auto const position{session->player_actor().body().movement().position};
  auto const bases{session->ground_actors()};
  bool const nearby{std::ranges::any_of(bases,[&](auto const &base){
    return base.side==0&&fast_distance(base.actor.placement().translation-position)<40000;
  })};
  if(!nearby){
    // 5b3be draws once before the first music-selection/random consumer.
    draw();finish_launch();return;
  }
  // Original enters 57ffc with draw/other pages 0/2 after the launch present.
  // The odd 115-frame lift restores 2/0 for the first simulation frame.
  transitions.present();
  startup.emplace(display_colours);display_colours=startup->colours();
  display_device.changed(32,16);display_device.changed(64,16);display_device.present(display_colours);
  startup_height=camera.position.z;
  clear_input();audio.play(33);
}
void mission_view::draw_startup(uint32_t now){
  if(!startup||startup_draw_pending)return;
  update_interface_clock(now);
  startup->prepare_frame(ui_random);
  camera.position.z=startup_height+startup->height_offset();
  draw();startup_draw_pending=true;
}
void mission_view::finish_startup_frame(uint32_t palette_clock,uint32_t end_clock){
  if(!startup||!startup_draw_pending)return;
  startup_draw_pending=false;
  if(startup->lifting()){
    if(startup->update_palette(palette_clock,startup_cover)){
      // 287a8 writes the complete fade range, including unchanged colours.
      // That full DAC upload also restores any earlier pre-write side effect.
      display_colours=startup->colours();display_device.changed(0,256);
    }
    display_device.present(display_colours);
    startup_page=transitions.snapshot().other_page*160;
    transitions.present();
    startup->presented(end_clock);
    if(!startup->lifting()){audio.stop(33);audio.play(41);}
  }else{
    startup->drawn(end_clock);
    // 57ffc never calls 59b40 in this loop: scan out the last lift page.
    surface.sample(frame,startup_page,startup->displacement());
    if(startup->finished()){startup.reset();clear_input();finish_launch();}
  }
}
bool mission_view::step_startup(){
  if(!startup||!pacing.take())return false;
  auto const now{pacing.state().slow};
  draw_startup(now);finish_startup_frame(now,now);return true;
}
void mission_view::advance_time(unsigned milliseconds){
  auto const before{elapsed_milliseconds*120/1000};elapsed_milliseconds+=milliseconds;
  auto const ticks{static_cast<unsigned>(elapsed_milliseconds*120/1000-before)};
  pacing.advance(ticks);audio.advance(ticks);
}
bool mission_view::step(bool const control, bool const zoom,mission_dialog_handlers const &dialogs) {
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
  return step_input(input,packet.hat,*ratio,routing.external,dialogs);
}
void mission_view::prepare_frame(uint32_t now){
  finish_launch();
  audio.update_music(session->music_observations(),ui_random);
  // 58574 reads cockpit view 4, which can precede camera ownership by a frame.
  audio.engine(transitions.snapshot().view==4);
  audio.flush_positions(camera.position);
  update_interface_clock(now);
  // Original 4cb14 consumes the mouse queue before world simulation.
  consume_pointer_events();
}
void mission_view::update_interface_clock(uint32_t now){
  auto const origin{instrument_origin()};auto const pages{transitions.snapshot()};
  clock.update_retained(now,pages.draw_page/2,pages.other_page/2,
    clock.visible_at(origin[0],origin[1],camera.mode!=camera_mode::attached),pages.prepared);
  if(now!=interface_ticks)lamps.advance(static_cast<int16_t>(now-interface_ticks));
  interface_ticks=now;
}
bool mission_view::step_recorded(tape_frame const &record,uint32_t slow_clock,bool suppress_camera,mission_dialog_handlers const &dialogs){
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
  return step_input(input,packet.hat,static_cast<frame_ratio>(record.delta),routing.external,dialogs);
}
bool mission_view::step_input(player_input initial_input,std::array<uint8_t,4> const &hat,frame_ratio ratio,bool control,mission_dialog_handlers const &dialogs){
  auto dispatch=[&](player_input &input){
  if(input.command && dialogs.command){
    switch(dialogs.command(input.command)){
      case mission_command_result::exit_mission:return false;
      case mission_command_result::handled:input.command=0;break;
      case mission_command_result::unhandled:break;
    }
  }
  if(input.command) {
    if(input.command==47){external_view_requested=!external_view_active;transitions.request(external_view_requested?2:3);input.command=0;}
    if((input.command==15||input.command==28)&&camera.mode!=camera_mode::attached){external_camera_control=!external_camera_control;input.command=0;}
    // 38034 routes TRACK through the widget. The bank reads the pending
    // flag during the later instrument exchange, after this frame's aiming.
    if(input.command==0x214&&camera.mode==camera_mode::attached&&panel!=cockpit_view::lower){session->weapon_group_control(4);audio.play(17);input.command=0;}
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
  flush_interface_audio();
  hat_view(hat);
  input.release_action=static_cast<control_release_action>(routing.mode);
  return true;
  };
  auto const result{session->step_player(initial_input, ratio, control, cockpit.layout.bounds(0x450),dispatch,[&](auto const &actions){
    steer_camera(camera,actions.zoom,actions.camera.y,actions.camera.x);
  })};
  if(!result)return false;
  if(dialogs.outcome)if(auto const prompt{session->take_mission_prompt()};prompt&&dialogs.outcome(*prompt))return false;
  painted_throttle_boundary=instruments.at(0).boundary;
  auto const throttle_request{session->exchange_player_instruments(requested_throttle, shield_to_front)};
  shield_to_front.reset();
  if(requested_throttle != throttle_request) {
    requested_throttle = throttle_request;
    instruments.set_value(0, requested_throttle);
  }
  session->refresh_target_instruments(camera.mode==camera_mode::attached&&panel==cockpit_view::forward,true);
  session->update_player_interface();
  // 2cf28 paints weapon widgets before 155a8/cf78 exchanges their next state.
  // Keep presentation one exchange behind without delaying simulation controls.
  auto const previous_weapon_rows{session->player_weapon_controls().displays()};
  painted_weapon_rows.assign(previous_weapon_rows.begin(),previous_weapon_rows.end());
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
  if(transitions.snapshot().prepared){
    auto const requested{external_view_requested?camera_mode::orbit:camera_mode::attached};
    if(camera.mode!=requested)set_camera(requested,impact_view().mode,false);
  }
  update_camera(camera, session->view_target());
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
  impact.update(interface_ticks,impact_view(),ui_random);
  for(auto const &request:session->take_sounds())audio.dispatch(request,camera.position);
  // Camera ownership changes in 5894c after this frame's player controls,
  // before the later 22904/226f0 presentation can prepare another view.
  if(transitions.snapshot().prepared)external_view_active=external_view_requested;
  present_pending=true;
  return true;
}

void mission_view::exchange_panel_values(){
  auto const &player{session->player_actor()};
  auto const &shield{player.power().shields};
  painted_shields={static_cast<int32_t>(shield.front)*1024/shield.capacity,
                  static_cast<int32_t>(shield.rear)*1024/shield.capacity,shield.balance};
  painted_reactor=static_cast<int32_t>(player.power().reserve)*1024/herc_power_capacity;
  auto const &movement{player.body().movement()};auto const &profile{player.profile().movement};
  int const capacity{movement.gait_rate<0?-profile.reverse_rate:profile.forward_rate};
  painted_speed=static_cast<int32_t>(movement.gait_rate)*1024/capacity;
}
void mission_view::update_widgets(){
  auto const origin{instrument_origin()};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  hdd->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->prepare(*session,camera,interface_ticks,origin[0],origin[1],transitions.snapshot().prepared&&transitions.snapshot().pending==3);
}
void mission_view::flush_interface_audio(){
  for(auto const id:displays->take_sounds())audio.play(id);
  for(auto const id:radio->take_sounds())audio.play(id);
  for(auto const id:hdd->take_sounds())audio.play(id);
}
void mission_view::begin_options(){
  if(options_camera)return;
  clear_input();options_camera=camera;
  // Original 57820 drains any outstanding pan before preparing the orbit
  // preview. Each preparation draws the frozen world and its interface.
  while(transitions.snapshot().pending!=-1||transitions.snapshot().prepared){
    present_pending=true;present_view();update_camera(camera,session->view_target());draw();
  }
  options_view=transitions.snapshot().view;
  auto preview=[&]{
    camera=session->make_camera(camera_mode::orbit);camera.orbit_distance=0;
    update_camera(camera,session->view_target());
  };
  if(options_view!=4){
    transitions.request(2);bool prepared{};
    do{
      present_pending=true;present_view();
      if(!prepared&&transitions.snapshot().prepared){prepared=true;preview();}
      update_camera(camera,session->view_target());draw();
    }while(!prepared||transitions.snapshot().prepared);
  }else{preview();draw();}
}
void mission_view::step_options_recorded(){
  if(!options_camera)return;
  // Original 577f8: fixed camera-turn request, no world simulation.
  steer_camera(camera,false,0,-64);update_camera(camera,session->view_target());
}
bool mission_view::step_options(){
  if(!options_camera||!pacing.take())return false;
  update_interface_clock(pacing.state().slow);
  step_options_recorded();return true;
}
void mission_view::end_options(){
  if(!options_camera)return;
  camera=*options_camera;options_camera.reset();
  if(options_view!=4){
    transitions.reset(options_view);panel=static_cast<cockpit_view>(options_view);
    suppress_interface=true;draw();
    present_pending=true;present_view();transitions.reset(options_view);
    suppress_interface=false;draw();
  }else{
    transitions.reset(4);draw();present_pending=true;present_view();
  }
  clear_input();
}
void mission_view::draw() {
  pan_frames.clear();
  frame.clear();
  if(!suppress_interface)update_widgets();
  auto const painted{painted_panel()};
  bool const inside{camera.mode == camera_mode::attached};
  auto const &window{cockpit.windows[static_cast<size_t>(painted)]};
  auto const shift{inside?(startup?startup->displacement():impact.snapshot().displacement):0};
  auto mask{window.mask};
  if(shift)mask.translate(0,-shift);
  auto &colours{impact.snapshot().impact_palette?impact_colours:display_colours};
  model_view view;
  view.cull_bounds=true;
  if(options_camera){view.bounds={0,0,319,110};view.projection.centre={160,50};}
  else if(!inside){view.bounds=external_view_artwork::bounds;view.projection.centre=external_view_artwork::centre;}
  if(inside) {
    cockpit.draw_composed(frame, painted,shift);
    auto const presentation{transitions.snapshot()};
    int const page{presentation.other_page};
    surface.restore_artwork_edge(frame,page*160,window.instrument_origin[1]+shift);
    auto const centre{window.projection_centre()};
    view.projection.centre = {centre[0], centre[1]};
    view.bounds = window.bounds;
    if(startup){
      view.projection.centre.y-=shift;
      view.bounds.top-=shift;view.bounds.bottom-=shift;
      if(startup->lifting()){
        startup_cover=startup->cover_y(camera.position.z,view.projection);
        view.bounds.bottom=std::clamp(startup_cover,view.bounds.top,view.bounds.bottom);
      }
    }
  }
  if(!inside)mask=external_view_artwork::clipping(view.bounds);
  // 59c8f skips the world while the lower panel is stable. The transition
  // still draws it, but idle lower-panel frames do not advance cloud wind.
  if(transitions.snapshot().view!=1||transitions.snapshot().prepared) {
    scoped_framebuffer_clip clip{frame, &mask};
    // 25b68 passes the cockpit's authored layout offset to 26264. The
    // projection centre alone leaves uncovered sky in an off-centre side view.
    auto const background_offset{inside?vec2<int32_t>{window.instrument_origin[0],window.instrument_origin[1]}:vec2<int32_t>{0,0}};
    environment.draw(frame,camera.position,camera.angles,view.projection,view.bounds,ui_random,colours,detail,background_offset,impact.snapshot().flash,!startup);
    if(!startup)display_device.changed(64,16);
    auto const objects{session->visible_objects(!inside)};
    auto &effects{session->visual_effects()};
    auto const ground{session->ground_models()};
    renderer.draw_objects(camera.position, camera.angles, {}, false, objects, &effects.lights(), ground, view);
  }
  session->refresh_target_instruments(inside&&panel==cockpit_view::forward);
  if(inside) {
    auto const &player{session->player_actor()};
    auto const origin{instrument_origin()};
    int const x{-origin[0]}, y{-origin[1]};
    lamps.draw(frame, x, y);
    clock.draw_retained(frame,transitions.snapshot().other_page/2,x,y);
    reactor.draw(frame, painted_reactor, x, y);
    instruments.painted(0,painted_throttle_boundary);
    painted_throttle_boundary.reset();
    throttle.draw_retained_at(frame, painted_speed, instruments.at(0).painted_thumb, x, y);
    auto const &shield_state{painted_shields};
    // 392d0 dirties the instrument only when one of these three values changes;
    // only its first repaint (391ee) uploads the six shield DAC entries.
    if(displayed_shields!=shield_state){
      displayed_shields=shield_state;update_shield_colours(colours,shield_state[0],shield_state[1]);
      display_device.changed(80,6);
    }
    shields.draw(frame, painted_shields[2], {std::to_underlying(instruments.at(1).face), std::to_underlying(instruments.at(2).face)}, x, y);
    for(auto const &binding : weapon_controls) {
      auto const &row{painted_weapon_rows.at(binding.row)};
      std::optional<int> boundary;
      if(binding.slider) { instruments.painted(*binding.slider); boundary = instruments.at(*binding.slider).boundary; }
      weapons.draw_retained(frame, row.control_number, row.kind, row.name, row.state, std::to_underlying(instruments.at(binding.button).face), boundary, x, y);
    }
    std::array<unsigned int, 5> faces;
    for(size_t i{}; i < faces.size(); ++i) faces[i] = std::to_underlying(instruments.at(weapon_groups[i]).face);
    weapons.draw_groups(frame, session->player_weapon_controls().current_group(), faces, x, y);
    navigation.draw(frame, x, y);
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
    // 35060 paints the ATT label after every HUD child, including waypoints.
    if(session->player_weapon_controls().auto_tracking()) navigation.draw_tracking(frame, x, y);
  }
  if(!inside&&!options_camera)external_artwork.draw(frame,external_camera_control);
  radio->advance_queue(*session,interface_ticks);
  radio->draw(frame);
  flush_interface_audio();
  if(startup&&startup->lifting()){
    scoped_framebuffer_clip clip{frame,&mask};
    frame.image(lift_artwork,0,startup_cover);
    frame.fill({window.bounds.left,startup_cover+static_cast<int>(lift_artwork.height),window.bounds.right,window.bounds.bottom},63);
    // 582b9 repaints the HUD after the lift door has covered the scene.
    auto const origin{instrument_origin()};
    navigation.draw(frame,-origin[0],-origin[1],session->player_weapon_controls().auto_tracking());
  }
  if(!options_camera){
    auto const state{transitions.snapshot()};
    // The original +88/+8c pair is swapped after scanout; the actual
    // driver draw selector (+3c) then targets the other page.
    int const page{state.other_page};
    int const y{inside?cockpit.windows[static_cast<size_t>(painted)].instrument_origin[1]:0};
    surface.store(frame,page*160,y);
  }
  auto const next_weapon_rows{session->player_weapon_controls().displays()};
  painted_weapon_rows.assign(next_weapon_rows.begin(),next_weapon_rows.end());
  exchange_panel_values();
  if(!startup){display_device.present(colours);present_view();}
}

int mission_view::pointer_cursor(int x,int y)const{
  if(startup||camera.mode!=camera_mode::attached)return -1;
  if(auto const edge{cockpit_edge_cursor(static_cast<int>(panel),x,y)})return edge;
  if(panel==cockpit_view::lower)return hdd->state().cursor?2:0;
  auto bounds{cockpit.layout.bounds(0x48c)};auto const parent{cockpit.layout.bounds(0x450)};
  auto const origin{instrument_origin()};
  bounds.left+=parent.left;bounds.right+=parent.left;bounds.top+=parent.top;bounds.bottom+=parent.top;
  return bounds.contains(x+origin[0],y+origin[1])?1:0;
}

std::optional<vec2<int32_t>> mission_view::take_pointer_warp(){return hdd->take_warp();}
std::vector<int16_t> mission_view::take_audio(){return audio.take_output();}
std::vector<framebuffer> mission_view::take_pan_frames(){return std::exchange(pan_frames,{});}
framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return display_device.colours(); }

} // namespace earthsiege
