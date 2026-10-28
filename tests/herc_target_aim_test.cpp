#include <cstdlib>
#include <algorithm>
#include <array>
#include <iostream>
#include "resources/binary_reader.h"
#include "simulation/herc_aiming.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHTA001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; std::array<size_t, 3> choices{};
  for(unsigned int i{0}; i != count; ++i) {
    auto const kind{static_cast<combat_actor_kind>(file.integer<uint8_t>())};
    vec3<int32_t> const origin{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const heading{file.integer<uint16_t>()}; auto const limit{file.integer<int16_t>()};
    vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const component{file.integer<int16_t>()};
    vec3<int32_t> const centre{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    vec3<int32_t> part; if(component >= 0) part = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    bool const aiming{file.integer<uint8_t>() != 0};
    vec3<int32_t> expected; if(aiming) expected = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const selection{select_herc_aim_point(kind, origin, heading, limit, position, component)};
    ++choices[static_cast<size_t>(selection.mode)];
    if(aiming == (selection.mode == herc_aim_mode::centre_controls)) file.fail(std::to_string(i) + " torso gate");
    if(!aiming) continue;
    auto const &actual{selection.mode == herc_aim_mode::target_component ? part : centre};
    if(actual != expected || (selection.mode == herc_aim_mode::target_component && selection.component != component)) file.fail(std::to_string(i) + " aim point");
  }
  file.require_end(); if(count != 3072 || std::any_of(choices.begin(), choices.end(), [](size_t n){return n == 0;})) file.fail("aim target coverage");
  std::cout << count << " original HERC torso gates and aim-point choices agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
