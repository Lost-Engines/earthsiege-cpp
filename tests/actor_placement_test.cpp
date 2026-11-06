#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/actor_placement.h"

int main(int const argc, char **argv) try {
  if(argc != 3 && argc != 4) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  earthsiege::placement_catalogue catalogue{resources};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPLACE1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int actors{0}, groups{0}, undefined{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const require{[&](bool const pass, std::string const &field){
      if(!pass) throw std::runtime_error{name + ": " + field + " differs"};
    }};
    auto const script{earthsiege::read_mission_script(file.take(file.integer<uint32_t>()))};
    auto const player{earthsiege::read_player_roster(file.take(file.integer<uint32_t>()), resources.edition())};
    auto const actual{catalogue.place(script, player)};
    for(auto const &pool : actual.actors) {
      require(pool.size() == file.integer<uint16_t>(), "actor count");
      for(auto const &actor : pool) {
        require(actor.source == file.integer<uint16_t>(), "actor source");
        auto const label{"actor " + std::to_string(actor.source) + " "};
        require(actor.definition == file.integer<int16_t>(), label + "definition");
        bool const valid{file.integer<uint8_t>() != 0};
        require(actor.position.has_value() == valid, label + "defined position");
        if(valid) {
          for(unsigned int axis{0}; axis != 3; ++axis) require((*actor.position)[axis] == file.integer<int32_t>(), label + "position");
        } else ++undefined;
        for(unsigned int axis{0}; axis != 3; ++axis) require(actor.angles[axis] == file.integer<uint16_t>(), label + "angle");
        require(actor.group == file.integer<uint16_t>(), label + "group");
        require(actor.ordinal == file.integer<uint16_t>(), label + "ordinal");
        require(actor.height == file.integer<int16_t>(), label + "height");
        require(actor.player_roster == (file.integer<uint8_t>() != 0), label + "player roster");
        require(actor.player_controlled == (file.integer<uint8_t>() != 0), label + "controlled player");
        require(actor.weapons.size() == file.integer<uint16_t>(), label + "weapon count");
        for(auto const weapon : actor.weapons) require(weapon == file.integer<int16_t>(), label + "weapon slot");
        ++actors;
      }
    }
    require(actual.groups.size() == file.integer<uint16_t>(), "group count");
    for(auto const &group : actual.groups) {
      require(static_cast<int16_t>(group.kind) == file.integer<int16_t>(), "group kind");
      require(group.formation == file.integer<int16_t>(), "formation");
      require(group.argument == file.integer<int16_t>(), "group argument");
      require(group.heading == file.integer<uint16_t>(), "group heading");
      require(group.mode == file.integer<uint8_t>(), "group mode");
      require(static_cast<int16_t>(group.target) == file.integer<int16_t>(), "target kind");
      require(group.members.size() == file.integer<uint16_t>(), "member count");
      for(auto const &member : group.members) {
        require(static_cast<int16_t>(member.kind) == file.integer<int16_t>(), "member kind");
        require(member.slot == file.integer<uint16_t>(), "member slot");
      }
      ++groups;
    }
    require(static_cast<int16_t>(actual.player.kind) == file.integer<int16_t>(), "player kind");
    require(actual.player.slot == file.integer<uint16_t>(), "player slot");
  }
  file.require_end();
  if(argc == 3 && (count != 304 || actors != 8176 || groups != 4368 || undefined != 89)) throw std::runtime_error{"placement fixture coverage differs"};
  std::cout << count << " original missions, " << actors << " actors and " << groups << " groups agree; " << undefined << " memory-dependent positions remain explicit" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
