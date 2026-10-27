#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include "simulation/navigation.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

earthsiege::movement_request request(earthsiege::binary_reader &file) {
  return {file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
}

void check(earthsiege::movement_request const &a, earthsiege::movement_request const &b, std::string const &label) {
  if(a.flags != b.flags || a.speed != b.speed || a.turn != b.turn) throw std::runtime_error{label + " differs: speed " + std::to_string(a.speed) + '/' + std::to_string(b.speed) + ", turn " + std::to_string(a.turn) + '/' + std::to_string(b.turn)};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESNAV001") file.fail("fixture version");
  auto const routes{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != routes; ++row) {
    auto cursor{file.integer<int16_t>()};
    bool const closed{file.integer<uint8_t>() != 0};
    auto const speed{file.integer<int16_t>()};
    auto const heading{file.integer<uint16_t>()};
    auto const position{point(file)};
    std::vector<vec3<int32_t>> points;
    earthsiege::mission_route route;
    auto const count{file.integer<uint16_t>()};
    for(unsigned int i{0}; i != count; ++i) {
      points.push_back(point(file));
      route.points.push_back(static_cast<int16_t>(closed && i == count - 1U ? 0 : i));
    }
    auto const next{file.integer<int16_t>()};
    auto const expected{request(file)};
    check(earthsiege::follow_route(position, heading, speed, route, points, cursor), expected, "route " + std::to_string(row));
    if(cursor != next) file.fail("route cursor differs");
  }
  auto const formations{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != formations; ++row) {
    auto const leader{point(file)};
    auto const leader_heading{file.integer<uint16_t>()};
    auto const leader_speed{file.integer<int16_t>()};
    auto const position{point(file)};
    auto const heading{file.integer<uint16_t>()};
    auto const offset{point(file)};
    auto const expected{request(file)};
    check(earthsiege::follow_formation(position, heading, leader, leader_heading, leader_speed, offset), expected, "formation " + std::to_string(row));
  }
  auto const avoidance{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != avoidance; ++row) {
    earthsiege::avoidance_state state;
    state.position = point(file);
    state.heading = file.integer<uint16_t>();
    state.current_speed = file.integer<int16_t>();
    state.collision_countdown = file.integer<int32_t>();
    state.forward_collision = file.integer<uint8_t>() != 0;
    state.collision_bias = file.integer<int16_t>();
    auto control{request(file)};
    auto expected{control};
    std::array<std::optional<int32_t>, 4> hits;
    for(auto &hit : hits) {
      bool const exists{file.integer<uint8_t>() != 0};
      auto const value{file.integer<int32_t>()};
      if(exists) hit = value;
    }
    std::vector<earthsiege::contact_actor> actors;
    auto const count{file.integer<uint16_t>()};
    for(unsigned int i{0}; i != count; ++i) {
      earthsiege::contact_actor actor;
      actor.position = point(file);
      actor.radius = file.integer<int16_t>();
      actor.waiting = file.integer<uint8_t>() != 0;
      actors.push_back(actor);
    }
    auto const ray_count{file.integer<uint8_t>()};
    std::vector<std::array<vec3<int32_t>, 2>> rays;
    for(unsigned int i{0}; i != ray_count; ++i) rays.push_back({point(file), point(file)});
    expected.speed = file.integer<int16_t>();
    expected.turn = file.integer<int16_t>();
    unsigned int visited{0};
    earthsiege::avoid_obstacles(control, state, actors, [&](earthsiege::avoidance_probe const kind, vec3<int32_t> const &a, vec3<int32_t> const &b){
      if(visited >= rays.size() || static_cast<unsigned int>(kind) != visited % 2 || a != rays[visited][0] || b != rays[visited][1]) file.fail("avoidance probe differs");
      return hits.at(visited++);
    });
    if(visited != rays.size()) file.fail("avoidance probe count differs");
    check(control, expected, "avoidance " + std::to_string(row));
  }
  file.require_end();
  std::cout << routes << " route, " << formations << " formation and " << avoidance << " avoidance decisions agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
