#include "mission_results.h"
#include "resources/binary_writer.h"
#include <stdexcept>
namespace earthsiege {
mission_results read_mission_results(bytes source,game_edition edition) {
  binary_reader file{source,"mission results"};mission_results out;out.grade=file.integer<int16_t>();out.recovery=file.integer<int32_t>();
  auto const count{file.integer<int16_t>()};if(count<0)file.fail("negative salvage count");
  for(int i{};i<count;++i){auto const weapon{file.integer<int16_t>()},condition{file.integer<int16_t>()};if(weapon<0||static_cast<size_t>(weapon)>=weapon_count(edition))file.fail("invalid salvaged weapon");out.salvage.emplace_back(weapon,condition);}
  if(file.remaining()%72||file.remaining()>4*72)file.fail("invalid return roster length");
  while(file.remaining()) {returned_herc h;
    for(auto &v:h.condition.armour)v=file.integer<int16_t>();
    for(auto &v:h.condition.internal)v=file.integer<int16_t>();
    for(auto &v:h.condition.equipment)v=file.integer<int16_t>();
    for(auto &v:h.statistics)v=file.integer<int16_t>();
    out.crew.push_back(h);
  }return out;
}
std::vector<uint8_t> write_mission_results(mission_results const &result) {
  if(result.salvage.size()>32767||result.crew.size()>4)throw std::runtime_error{"result exceeds original pools"};
  binary_writer file;file.integer(result.grade);file.integer(result.recovery);file.integer<int16_t>(static_cast<int16_t>(result.salvage.size()));
  for(auto [weapon,condition]:result.salvage){file.integer(weapon);file.integer(condition);}
  for(auto const &h:result.crew){for(auto v:h.condition.armour)file.integer(v);for(auto v:h.condition.internal)file.integer(v);for(auto v:h.condition.equipment)file.integer(v);for(auto v:h.statistics)file.integer(v);}
  return std::move(file).finish();
}
} // namespace earthsiege
