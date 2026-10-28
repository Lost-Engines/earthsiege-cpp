#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  auto const bullets{earthsiege::read_bullet_motions(resources.read("simvol0", "dat/BULLETS.DAT"))};
  auto const rockets{earthsiege::read_rocket_motions(resources.read("simvol0", "dat/ROCKETS.DAT"))};
  int lifetime{0};
  for(auto const &motion : bullets) lifetime = std::max(lifetime, static_cast<int>(motion.lifetime));
  for(auto const &motion : rockets) lifetime = std::max(lifetime, static_cast<int>(motion.lifetime));
  size_t rendered{0}, live_shots{0}, live_effects{0};
  for(unsigned int demo{1}; demo != 5; ++demo) {
    auto const tape{earthsiege::read_input_tape(earthsiege::read_file(resources.root() / "TAPES" / ("DEMO" + std::to_string(demo) + ".TAP")))};
    auto script{earthsiege::read_mission_script(tape.starting_files[std::to_underlying(earthsiege::tape_file::script)])};
    // Isolate the player firing/effect lifecycle from autonomous hostile fire.
    script.header.unlimited_weapons = 0;
    for(auto &group : script.groups) group.mode = 0;
    auto const roster{earthsiege::read_player_roster(tape.starting_files[std::to_underlying(earthsiege::tape_file::player)])};
    earthsiege::mission_session session{resources, script, roster};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, session.scenery(), bindings};
    auto camera{session.make_camera(earthsiege::camera_mode::orbit)};
    camera.orbit_angles.x = 0xf800;
    auto const ammunition{[&]{
      int count{0};
      for(auto const &slot : session.player_actor().weapons().slots()) if(slot && slot->ammunition()) count += slot->ammunition()->rounds;
      return count;
    }};
    auto const initial_ammunition{ammunition()};
    bool fired{false}, saved{false};
    for(size_t slot{0}; slot != session.player_actor().weapons().slots().size(); ++slot) {
      if(!session.select_weapon(slot)) continue;
      for(unsigned int step{0}; step != 64; ++step) {
        session.step({.elevation{static_cast<int16_t>(step < 16 ? -256 : 0)}}, 256, step >= 16);
        auto &effects{session.visual_effects()};
        auto const shots{session.ordnance().live().size()};
        live_shots += shots; live_effects += effects.live_explosions().size();
        fired |= shots != 0;
        if(step % 8 == 0 || (!saved && effects.live_explosions().size() != 0)) {
          earthsiege::update_camera(camera, session.view_target());
          auto const objects{session.visible_objects()}; auto const ground{effects.ground_flashes()};
          frame.clear();
          renderer.draw_objects(camera.position, camera.angles, {}, false, objects, &effects.lights(), ground);
          ++rendered;
          if(!saved && effects.live_explosions().size() != 0) { frame.save("combat-demo" + std::to_string(demo) + ".ppm", session.scenery().colours()); saved = true; }
        }
      }
    }
    if(!fired) throw std::runtime_error{"demo " + std::to_string(demo) + " never created a projectile"};
    if(initial_ammunition > 0 && ammunition() >= initial_ammunition) throw std::runtime_error{"firing did not consume ammunition"};
    for(int step{0}; step != lifetime + 32; ++step) session.step({}, 256);
    if(session.ordnance().live().size() != 0 || session.visual_effects().live_explosions().size() != 0 || session.visual_effects().live_particles().size() != 0) throw std::runtime_error{"combat effects did not retire after fire released"};
  }
  if(live_effects == 0) throw std::runtime_error{"no shot reached the world impact/effect path"};
  std::cout << "Four demo missions exercised firing, consumption, impacts and reclamation: " << rendered << " rendered frames, " << live_shots << " live projectile observations and " << live_effects << " explosion observations\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
