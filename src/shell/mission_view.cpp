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
  return std::make_unique<mission_session>(resources, script, roster);
}

} // namespace

mission_view::mission_view(resource_store const &resources, unsigned int const demo)
  : session{demo_start(resources, demo)},
    renderer{frame, session->scenery(), read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))},
    camera{session->make_camera(camera_mode::orbit)} {
  camera.orbit_angles.x = 0xf000;
  update_camera(camera, session->view_target());
}

void mission_view::key(held_key const key, bool const pressed, bool const alt) {
  keyboard.event(key, pressed, alt);
}

void mission_view::clear_input() {
  keyboard.clear();
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
  auto const input{keyboard.sample()};
  if(paused) return;
  if(control) steer_camera(camera, zoom, static_cast<int16_t>(input.movement.y), static_cast<int16_t>(input.movement.x));
  else {
    steer_camera(camera, false, 0, 0);
    session->step({
      .throttle{static_cast<int16_t>(input.movement.y)},
      .steering{static_cast<int16_t>(input.movement.x)},
      .torso{static_cast<int16_t>(input.aiming.x)},
      .elevation{static_cast<int16_t>(input.aiming.y)},
    }, ratio_from_period(25, nominal_frame_milliseconds));                       // fixed inspection cadence; full mission scheduling is separate
  }
  update_camera(camera, session->view_target());
}

void mission_view::draw() {
  frame.clear();
  background_view const background{.pitch{camera.angles.x}, .roll{camera.angles.y}, .offset{0, 0}};
  draw_background_bands(frame, make_background_bands(session->scenery().environment(), background), background.bounds);
  std::array const lights{default_world_light()};
  renderer.draw(camera.position, camera.angles, lights, false, session->roster_models());
}

framebuffer const &mission_view::image() const { return frame; }
palette const &mission_view::colours() const { return session->scenery().colours(); }

} // namespace earthsiege
