#include "player_roster.h"
#include <algorithm>
#include <utility>

namespace earthsiege {

player_roster read_player_roster(bytes const source, game_edition const edition) {
  binary_reader file{source, "player roster"};
  player_roster result;
  result.edition = edition;
  result.selected = file.integer<int16_t>();
  auto const count{file.integer<int16_t>()};
  if(count < 0 || count > 50) file.fail("player roster exceeds original pool");
  for(int index{0}; index != count; ++index) {
    player_herc herc;
    herc.slot = file.integer<int16_t>();
    herc.field = file.integer<int16_t>();
    herc.chassis = file.integer<int16_t>();
    auto const weapons{file.integer<int16_t>()};
    if(weapons < 0 || weapons > 12) file.fail("player weapon count exceeds original record");
    for(int weapon{0}; weapon != weapons; ++weapon) herc.weapons.push_back(file.integer<int16_t>());
    for(int weapon{0}; weapon != weapons; ++weapon) herc.weapon_values.push_back(file.integer<int16_t>());
    herc.extra = file.integer<int16_t>();
    for(auto &value : herc.condition.armour) value = file.integer<int16_t>();
    for(auto &value : herc.condition.internal) value = file.integer<int16_t>();
    for(auto &value : herc.condition.equipment) value = file.integer<int16_t>();
    result.hercs.push_back(std::move(herc));
  }
  if(edition == game_edition::expansion) {
    auto const count_flags{file.integer<uint16_t>()};
    if(count_flags > 256) file.fail("player availability table too large");
    result.flags.resize(count_flags);
  }
  std::ranges::copy(file.take(result.flags.size()), result.flags.begin());
  file.require_end();
  if(result.selected < 0 || static_cast<size_t>(result.selected) >= result.hercs.size()) file.fail("selected player is outside roster");
  return result;
}

} // namespace earthsiege
