#include "mission_view.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include "render/background_bands.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/frame_time.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

std::unique_ptr<mission_session> demo_start(resource_store const &resources, unsigned int const demo) {
  if(demo < 1 || demo > 4) throw std::runtime_error{"--inspect-demo must be between 1 and 4"};
  auto const tape{read_input_tape(read_file(resources.root() / "TAPES" / ("DEMO" + std::to_string(demo) + ".TAP")))};
  auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
  auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
  auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
  return std::make_unique<mission_session>(resources, script, roster, random_state{}, counters);
}

} // namespace

mission_view::mission_view(resource_store const &resources, unsigned int const demo)
  : session{demo_start(resources, demo)},
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
    renderer{frame, session->scenery(), read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))},
    camera{session->make_camera(camera_mode::attached)} {
  apply_cockpit_colours(display_colours, read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL")), static_cast<unsigned int>(session->player_chassis()));
  update_camera(camera, session->view_target());
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
  auto const &origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

void mission_view::key(held_key const key, bool const pressed, bool const alt) {
  keyboard.event(key, pressed, alt);
}

void mission_view::command(uint16_t const code) { commands.push_back(code); }
void mission_view::mouse_position(int const x, int const y) { pointer.mouse = {x,y}; pointer_events.push_back(pointer); }
void mission_view::mouse_button(bool const right, bool const pressed) {
  (right ? pointer.right : pointer.left) = pressed;
  pointer_events.push_back(pointer);
}

void mission_view::consume_pointer_events() {
  auto const &origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin};
  instruments.window_bounds({origin[0], origin[1], origin[0] + 319, origin[1] + 199});
  if(camera.mode == camera_mode::attached) for(auto const &event : pointer_events) {
    if(auto const action{instruments.event(event.mouse.x + origin[0], event.mouse.y + origin[1], event.left, event.right)}) {
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

void mission_view::change_camera(camera_mode const mode) {
  if(mode == camera_mode::free) camera.mode = mode;                             // detach at the current position and orientation
  else camera = session->make_camera(mode);
  clear_input();
  update_camera(camera, session->view_target());
  auto const &player{session->player_actor()};
  navigation.update(static_cast<int16_t>(player.body().movement().angles.z), player.body().torso().angle,
                    player.body().elevation().angle, player.damage().consequences.disabled);
}

void mission_view::change_cockpit_view(cockpit_view const view) {
  if(panel != view) clock.refresh();
  panel = view;
  change_camera(camera_mode::attached);
  auto const &origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
}

void mission_view::step(bool const control, bool const zoom) {
  if(paused) { pointer_events.clear(); return; }
  // The original 120 Hz timer invokes both UI clocks every second callback.
  // Accumulate the nominal 60 Hz ticks separately from the 40 Hz physics loop.
  elapsed_milliseconds += 25;
  auto const now{static_cast<uint32_t>(elapsed_milliseconds * 60 / 1000)};
  clock.update(now);
  if(now != interface_ticks) lamps.advance(static_cast<int16_t>(now - interface_ticks));
  interface_ticks = now;
  consume_pointer_events();
  auto const keys{keyboard.sample(session->player_weapon_controls().manual_guidance())};
  player_input input{pointer};
  input.movement = {static_cast<int16_t>(keys.movement.x),static_cast<int16_t>(keys.movement.y)};
  input.aiming = {static_cast<int16_t>(keys.aiming.x),static_cast<int16_t>(keys.aiming.y)};
  input.camera_source = player_axis_pair::movement;
  input.fire = keys.fire || (control && zoom);
  input.enter = keys.enter;
  if(!commands.empty()) {
    input.command=commands.front(); commands.pop_front();
    if(input.command>=59 && input.command<=64 && camera.mode==camera_mode::attached && panel!=cockpit_view::forward)
      change_cockpit_view(cockpit_view::forward);
    if(displays->command(input.command,*session,instruments)) input.command=0;
  }
  auto const actions{session->step_player(input, ratio_from_period(25, nominal_frame_milliseconds), control, cockpit.layout.bounds(0x450))};
  steer_camera(camera, actions.zoom, actions.camera.y, actions.camera.x);
  auto const throttle_request{session->exchange_player_instruments(requested_throttle, shield_to_front)};
  shield_to_front.reset();
  if(requested_throttle != throttle_request) {
    requested_throttle = throttle_request;
    instruments.set_value(0, requested_throttle);
  }
  session->update_player_interface();
  session->exchange_weapon_instruments(ratio_from_period(25, nominal_frame_milliseconds));
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
  auto const &origin{cockpit.windows[static_cast<size_t>(panel)].instrument_origin};
  displays->update(*session,instruments,camera,interface_ticks,origin[0],origin[1]);
  radio->update(*session,camera,interface_ticks,origin[0],origin[1]);
}

void mission_view::draw() {
  frame.clear();
  bool const inside{camera.mode == camera_mode::attached};
  auto const &window{cockpit.windows[static_cast<size_t>(panel)]};
  model_view view;
  if(inside) {
    cockpit.draw(frame, panel);
    auto const centre{window.projection_centre()};
    view.projection.centre = {centre[0], centre[1]};
    view.bounds = window.bounds;
  }
  {
    scoped_framebuffer_clip clip{frame, inside ? &window.mask : nullptr};
    background_view const background{.pitch{camera.angles.x}, .roll{camera.angles.y}, .projection_shift{view.projection.perspective_shift}, .centre{vec2<int32_t>{view.projection.centre}}, .offset{0, 0}, .bounds{view.bounds}};
    draw_background_bands(frame, make_background_bands(session->scenery().environment(), background), background.bounds);
    auto const objects{session->visible_objects(!inside)};
    auto &effects{session->visual_effects()};
    auto const ground{effects.ground_flashes()};
    renderer.draw_objects(camera.position, camera.angles, {}, false, objects, &effects.lights(), ground, view);
  }
  if(inside) {
    auto const &player{session->player_actor()};
    auto const &movement{player.body().movement()};
    auto const &profile{player.profile().movement};
    int const x{-window.instrument_origin[0]}, y{-window.instrument_origin[1]};
    lamps.draw(frame, x, y);
    clock.draw(frame, x, y);
    reactor.draw(frame, static_cast<int32_t>(player.power().reserve) * 1024 / herc_power_capacity, x, y);
    int const speed_capacity{movement.gait_rate < 0 ? -profile.reverse_rate : profile.forward_rate};
    instruments.painted(0);
    throttle.draw_at(frame, static_cast<int32_t>(movement.gait_rate) * 1024 / speed_capacity, instruments.at(0).painted_thumb, x, y);
    auto const &shield{player.power().shields};
    update_shield_colours(display_colours, static_cast<int32_t>(shield.front) * 1024 / shield.capacity,
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
    if(auto const marker{session->target_marker()}) {
      auto camera_transform{rotation_transform(camera.angles)};
      camera_transform.translation = camera.position;
      if(auto const brackets{target_brackets(*marker, inverse_transform(camera_transform), view.projection)}) {
        scoped_framebuffer_clip target_clip{frame, &window.mask};
        draw_target_brackets(frame, *brackets, marker->locked, marker->side);
      }
    }
    waypoints.draw(frame, player.body().movement().position, player.body().movement().angles.z, navigation.display_heading(), session->next_waypoint(), x, y);
  }
  radio->draw(frame);
}

framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return display_colours; }

} // namespace earthsiege
