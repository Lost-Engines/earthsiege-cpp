#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/herc_body.h"
#include "simulation/component_damage.h"

namespace {

struct reference_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  std::array<int16_t, 8> values;
  std::vector<earthsiege::animation_pose> poses;
  std::vector<earthsiege::model_transform> matrices;
};

reference_state read_state(earthsiege::binary_reader &file) {
  reference_state result;
  for(unsigned int axis{0}; axis != 3; ++axis) result.position[axis] = file.integer<int32_t>();
  for(unsigned int axis{0}; axis != 3; ++axis) result.angles[axis] = file.integer<uint16_t>();
  for(auto &value : result.values) value = file.integer<int16_t>();
  auto const nodes{file.integer<uint16_t>()};
  result.poses.resize(nodes);
  result.matrices.resize(nodes);
  for(auto &pose : result.poses) {
    for(unsigned int axis{0}; axis != 3; ++axis) pose.rotation[axis] = file.integer<uint16_t>();
    for(unsigned int axis{0}; axis != 3; ++axis) pose.translation[axis] = file.integer<int16_t>();
  }
  for(auto &matrix : result.matrices) {
    for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) matrix.basis.data[field] = file.integer<int16_t>();
    matrix.type = file.integer<uint8_t>();
    file.integer<uint8_t>();
    for(unsigned int axis{0}; axis != 3; ++axis) matrix.translation[axis] = file.integer<int32_t>();
  }
  return result;
}

void check(earthsiege::herc_body const &body, reference_state const &expected, std::string const &label) {
  auto const require{[&](bool const pass, std::string const &field){
    if(!pass) throw std::runtime_error{label + ": " + field + " differs"};
  }};
  auto const &movement{body.movement()};
  require(movement.position == expected.position, "position");
  require(movement.angles == expected.angles, "angles");
  std::array<int16_t, 8> const values{movement.steering_rate, movement.gait_rate, movement.throttle, movement.playback_rate, body.torso().rate, body.elevation().rate, body.torso().angle, body.elevation().angle};
  for(size_t i{0}; i != values.size(); ++i) require(values[i] == expected.values[i], "control field " + std::to_string(i));
  require(body.model().poses().size() == expected.poses.size(), "pose count");
  for(size_t i{0}; i != expected.poses.size(); ++i) {
    auto const &a{body.model().poses()[i]};
    auto const &b{expected.poses[i]};
    require(a.rotation == b.rotation && a.translation == b.translation, "pose " + std::to_string(i));
    auto const &c{body.model().transforms()[i]};
    auto const &d{expected.matrices[i]};
    require(c.basis == d.basis && c.translation == d.translation && c.type == d.type, "transform " + std::to_string(i));
  }
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  std::string const version{magic.begin(), magic.end()};
  if(version != "ESBODY01" && version != "ESBODY02" && version != "ESBODY03" && version != "ESBODY04") file.fail("fixture version");
  bool const collisions{version == "ESBODY02"};
  auto const count{file.integer<uint16_t>()};
  unsigned int total{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    std::vector<int16_t> equipment(file.integer<uint16_t>());
    for(auto &value : equipment) value = file.integer<int16_t>();
    auto const initial{read_state(file)};
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_body body{definition, weapons, templates, equipment, initial.position, initial.angles};
    check(body, initial, name + "/initial");
    auto const steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != steps; ++step) {
      earthsiege::herc_controls const controls{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      auto const ratio{file.integer<int16_t>()};
      if(version == "ESBODY04") {
        earthsiege::damage_consequences consequences;
        consequences.disabled = file.integer<uint8_t>() != 0;
        consequences.movement_light = file.integer<uint8_t>() != 0;
        consequences.movement_heavy = file.integer<uint8_t>() != 0;
        auto const recovery{file.integer<int32_t>()};
        bool const reverse{file.integer<uint8_t>() != 0};
        auto const expected_recovery{file.integer<int32_t>()};
        bool const collapsed{file.integer<uint8_t>() != 0};
        auto const events{file.integer<uint8_t>()};
        std::vector<std::array<int16_t, 3>> expected_events;
        for(unsigned int i{0}; i != events; ++i) {
          auto const kind{file.integer<uint8_t>()};
          auto const a{file.integer<int16_t>()};
          auto const b{kind == 1 ? file.integer<int16_t>() : int16_t{0}};
          expected_events.push_back({kind, a, b});
        }
        body.apply_damage(consequences);
        if(recovery >= 0) body.recover_from_collision(recovery, reverse);
        std::vector<std::array<int16_t, 3>> actual_events;
        body.step_requested(controls.throttle, controls.steering, {controls.torso, controls.elevation}, ratio, {}, [&](earthsiege::movement_event const event){
          if(event == earthsiege::movement_event::collapse_started) actual_events.push_back({0, 0x1e, 0});
          else { actual_events.push_back({1, 120, 150}); actual_events.push_back({0, 0x29, 0}); }
        });
        if(body.movement().collision_recovery != expected_recovery || body.movement().collapsed != collapsed || actual_events != expected_events) file.fail(name + "/" + std::to_string(step) + ": damaged movement events/recovery differ");
      } else if(collisions) {
        std::array<bool, 2> const hits{file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0};
        bool const expected_hit{file.integer<uint8_t>() != 0};
        auto const probes{file.integer<uint8_t>()};
        unsigned int visited{0};
        bool const hit{body.step(controls, ratio, [&](earthsiege::herc_body const &candidate){
          if(visited >= probes) file.fail(name + "/" + std::to_string(step) + ": too many contact probes");
          vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
          vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
          if(candidate.movement().position != position || candidate.movement().angles != angles) file.fail(name + "/" + std::to_string(step) + ": contact placement differs at " + std::to_string(visited));
          return hits.at(visited++);
        })};
        if(visited != probes || hit != expected_hit) file.fail("contact response differs");
      } else if(version == "ESBODY03") body.step_requested(controls.throttle, controls.steering, vec2<int16_t>{controls.torso, controls.elevation}, ratio);
      else body.step(controls, ratio);
      check(body, read_state(file), name + "/" + std::to_string(step));
      ++total;
    }
  }
  file.require_end();
  if(count != 8 || total != (version == "ESBODY04" ? 2880U : collisions ? 1920U : 1280U)) file.fail("incomplete HERC body references");
  std::cout << total << " original combined movement, aiming, pose and hierarchy updates agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
