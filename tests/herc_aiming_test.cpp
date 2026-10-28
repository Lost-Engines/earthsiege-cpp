#include <cstdlib>
#include <iostream>
#include "resources/binary_reader.h"
#include "simulation/herc_aiming.h"
#include "simulation/aim_control.h"

namespace {
using namespace earthsiege;
model_transform read_matrix(binary_reader &file) {
  model_transform value;
  for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) value.basis.data[field] = file.integer<int16_t>();
  value.type = file.integer<uint8_t>(); file.integer<uint8_t>();
  value.translation = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
  return value;
}
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESH A001") file.fail("fixture version");
  auto const actors{file.integer<uint16_t>()}; size_t updates{0}, convergences{0};
  for(unsigned int actor{0}; actor != actors; ++actor) {
    auto const height{file.integer<int16_t>()};
    aiming_profile yaw, pitch;
    yaw.rate = file.integer<int16_t>(); yaw.acceleration = file.integer<int16_t>(); yaw.maximum = file.integer<int16_t>(); yaw.minimum = -yaw.maximum;
    pitch.rate = file.integer<int16_t>(); pitch.acceleration = file.integer<int16_t>(); pitch.maximum = file.integer<int16_t>(); pitch.minimum = file.integer<int16_t>();
    std::vector<vec3<int16_t>> offsets;
    auto const weapons{file.integer<uint16_t>()};
    for(unsigned int i{0}; i != weapons; ++i) if(file.integer<uint8_t>()) offsets.push_back(vec3<int16_t>{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()});
    auto const trials{file.integer<uint16_t>()};
    for(unsigned int trial{0}; trial != trials; ++trial) {
      auto const label{std::to_string(actor) + '/' + std::to_string(trial)};
      bool const centre{file.integer<uint8_t>() != 0}; auto const distance{file.integer<int32_t>()};
      vec3<int32_t> const point{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      auto const placement{read_matrix(file)}, pivot{read_matrix(file)}; auto const ratio{file.integer<int16_t>()};
      aiming_state ys, ps;
      ys.rate = file.integer<int16_t>(); ps.rate = file.integer<int16_t>(); ys.angle = file.integer<int16_t>(); ps.angle = file.integer<int16_t>();
      vec3<int16_t> const expected_errors{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      auto const torso_input{file.integer<int16_t>()}; auto const range{file.integer<int32_t>()}; auto const elevation_input{file.integer<int16_t>()};
      auto const request{centre ? centre_herc_aim(ys.angle, ps.angle, distance) : aim_herc_at(point, placement, pivot, height)};
      if(request.errors != expected_errors || request.controls != vec2<int16_t>{torso_input, elevation_input} || request.convergence_distance != range) file.fail(label + " aim request");
      update_aim(ys, yaw, request.controls.x, ratio); update_aim(ps, pitch, request.controls.y, ratio);
      if(ys.rate != file.integer<int16_t>() || ps.rate != file.integer<int16_t>() || ys.angle != file.integer<int16_t>() || ps.angle != file.integer<int16_t>()) file.fail(label + " aim controller");
      for(auto const &offset : offsets) {
        vec3<int16_t> const expected{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
        auto const actual{herc_weapon_convergence(offset, height, range)};
        if(actual != expected) file.fail(label + " weapon convergence " + std::to_string(actual.x) + "," + std::to_string(actual.z) + " expected " + std::to_string(expected.x) + "," + std::to_string(expected.z));
        ++convergences;
      }
      ++updates;
    }
  }
  file.require_end(); if(actors != 56 || updates != 1792 || convergences == 0) file.fail("aim coverage");
  std::cout << updates << " original aim updates and " << convergences << " weapon convergence angles agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
