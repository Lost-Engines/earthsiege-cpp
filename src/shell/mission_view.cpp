#include "mission_view.h"
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include "render/background_bands.h"
#include "resources/input_tape.h"
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
    renderer{frame, session->scenery(), read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))},
    camera{session->make_camera(camera_mode::orbit)} {
  camera.orbit_angles.x = 0xf800;
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
  background_view const background{.pitch{camera.angles.x}, .roll{camera.angles.y}, .offset{0, 0}};
  draw_background_bands(frame, make_background_bands(session->scenery().environment(), background), background.bounds);
  auto const objects{session->visible_objects()};
  auto &effects{session->visual_effects()};
  auto const ground{effects.ground_flashes()};
  renderer.draw_objects(camera.position, camera.angles, {}, false, objects, &effects.lights(), ground);
}

framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return session->scenery().colours(); }

} // namespace earthsiege
