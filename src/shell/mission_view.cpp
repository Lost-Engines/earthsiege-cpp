#include "mission_view.h"
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include "render/background_bands.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/frame_time.h"

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
    display_colours{session->scenery().colours()},
    renderer{frame, session->scenery(), read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))},
    camera{session->make_camera(camera_mode::attached)} {
  apply_cockpit_colours(display_colours, read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL")), static_cast<unsigned int>(session->player_chassis()));
  update_camera(camera, session->view_target());
}

void mission_view::key(held_key const key, bool const pressed, bool const alt) {
  keyboard.event(key, pressed, alt);
}

void mission_view::command(uint16_t const code) { commands.push_back(code); }
void mission_view::mouse_position(int const x, int const y) { pointer.mouse = {x,y}; }
void mission_view::mouse_button(bool const right, bool const pressed) {
  (right ? pointer.right : pointer.left) = pressed;
}

void mission_view::clear_input() {
  keyboard.clear();
  pointer.left = pointer.right = false;
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
}

void mission_view::change_cockpit_view(cockpit_view const view) {
  panel = view;
  change_camera(camera_mode::attached);
}

void mission_view::step(bool const control, bool const zoom) {
  if(paused) return;
  auto const keys{keyboard.sample(session->player_weapon_controls().manual_guidance())};
  player_input input{pointer};
  input.movement = {static_cast<int16_t>(keys.movement.x),static_cast<int16_t>(keys.movement.y)};
  input.aiming = {static_cast<int16_t>(keys.aiming.x),static_cast<int16_t>(keys.aiming.y)};
  input.camera_source = player_axis_pair::movement;
  input.fire = keys.fire || (control && zoom);
  input.enter = keys.enter;
  if(!commands.empty()) { input.command=commands.front(); commands.pop_front(); }
  auto const actions{session->step_player(input, ratio_from_period(25, nominal_frame_milliseconds), control)};
  steer_camera(camera, actions.zoom, actions.camera.y, actions.camera.x);
  session->update_player_interface();
  update_camera(camera, session->view_target());
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
    reactor.draw(frame, static_cast<int32_t>(player.power().reserve) * 1024 / herc_power_capacity, x, y);
    int const speed_capacity{movement.gait_rate < 0 ? -profile.reverse_rate : profile.forward_rate};
    throttle.draw(frame, static_cast<int32_t>(movement.gait_rate) * 1024 / speed_capacity, movement.throttle, x, y);
    auto const &shield{player.power().shields};
    update_shield_colours(display_colours, static_cast<int32_t>(shield.front) * 1024 / shield.capacity,
                         static_cast<int32_t>(shield.rear) * 1024 / shield.capacity);
    shields.draw(frame, shield.balance, {}, x, y);
  }
}

framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return display_colours; }

} // namespace earthsiege
