#include <algorithm>
#include <cstdlib>
#include <iostream>
#include "shell/mission_view.h"
using earthsiege::camera_mode;
using earthsiege::mission_view;
using earthsiege::resource_store;
using earthsiege::tape_frame;
using earthsiege::update_camera;
namespace {
void require(bool ok, char const *why) {
  if(!ok) throw std::runtime_error{why};
}
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  size_t switches{0};
  size_t wingmate_returns{0};
  for(unsigned demo{1}; demo <= 4; ++demo) {
    mission_view view{resources, demo};
    auto const player{view.simulation().player_id()};
    require(!view.simulation().objective_text.empty(), "demo restores its objectives");
    auto step{[&](uint16_t code){
      tape_frame f;
      f.delta = 1;
      f.command = code;
      require(view.step_recorded(f, 0), "camera command keeps mission running");
    }};
    step(49);
    require(view.camera_actor() == player, "N ignored in cockpit");
    view.change_camera(camera_mode::orbit);
    tape_frame turn;
    turn.delta = 1;
    turn.primary_axes.x = 256;
    require(view.step_recorded(turn, 0), "camera turn keeps mission running");
    step(0);
    auto const original_orbit{view.view_camera().orbit_angles};
    require(original_orbit.z != 0, "saved player camera has a distinct orbit");
    auto actors{view.simulation().hercs()};
    size_t current{player};
    for(size_t iteration{0}; iteration <= actors.size(); ++iteration) {
      size_t expected{current};
      for(size_t offset{1}; offset < actors.size(); ++offset) {
        auto next{(current + offset) % actors.size()};
        if(actors[next].group == actors[player].group && actors[next].actor.body().movement().position.z >= 0) {
          expected = next;
          break;
        }
      }
      step(49);
      require(view.camera_actor() == expected, "N follows original actor order within player group");
      current = expected;
      ++switches;
      auto reference{view.view_camera()};
      update_camera(reference, view.simulation().view_target(current));
      require(reference.position == view.view_camera().position, "camera follows selected squad HERC");
      if(current == player) {
        require(view.view_camera().orbit_angles == original_orbit, "player orbit restored after squad cycle");
        break;
      }
      require(view.view_camera().orbit_angles == vec3<uint16_t>{}, "newly bound squad camera starts at zero orbit");
    }
    step(49);
    auto const subject{view.camera_actor()};
    require(view.step_recorded(turn, 0), "turn remembered external camera");
    step(0);
    auto const orbit{view.view_camera().orbit_angles};
    view.change_camera(camera_mode::attached);
    require(view.camera_actor() == player, "returning to cockpit restores player target");
    step(49);
    view.change_camera(camera_mode::orbit);
    require(view.camera_actor() == subject, "reopening the external view restores its subject");
    require(view.view_camera().orbit_angles == orbit, "reopening the external view restores its orbit");
    if(subject != player) ++wingmate_returns;
  }
  require(switches >= 4, "squad cycle exercised");
  require(wingmate_returns > 0, "left and reopened a wingmate's camera");
  std::cout << switches << " squad camera switches and four restored objective lists pass\n";
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
