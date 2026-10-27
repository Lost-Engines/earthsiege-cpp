#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/aim_control.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESAIM001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::map<std::string, earthsiege::herc_profile> profiles;
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    if(!profiles.contains(name)) profiles.emplace(name, earthsiege::read_herc_profile(resources.read("simvol0", "dat/" + name + ".DAT")));
    bool const elevation{file.integer<uint8_t>() != 0};
    auto const ratio{file.integer<int16_t>()};
    auto const input{file.integer<int16_t>()};
    earthsiege::aiming_state state{.rate{file.integer<int16_t>()}, .angle{file.integer<int16_t>()}};
    earthsiege::aiming_state const expected{.rate{file.integer<int16_t>()}, .angle{file.integer<int16_t>()}};
    auto const &profile{profiles.at(name)};
    earthsiege::update_aim(state, elevation ? profile.elevation : profile.torso, input, ratio);
    if(state.rate != expected.rate || state.angle != expected.angle) throw std::runtime_error{"aim controller differs in " + name + " case " + std::to_string(row)};
  }
  file.require_end();
  if(count != 13440 || profiles.size() != 16) file.fail("incomplete aiming fixtures");
  std::cout << count << " original torso/elevation controls agree across all " << profiles.size() << " HERC profiles" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
