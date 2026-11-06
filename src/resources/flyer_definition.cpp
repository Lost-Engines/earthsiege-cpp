#include "flyer_definition.h"
#include <string>
#include "resources/resource_store.h"

namespace earthsiege {
flyer_definition read_flyer_definition(resource_store const &resources, std::string_view const name) {
  flyer_definition result;
  auto const source{resources.read("simvol0", "dat/" + std::string{name} + ".DAT")};
  binary_reader reader{source};
  if(resources.edition() == game_edition::expansion) result.profile.source.resize(23);
  for(auto &value : result.profile.source) value = reader.integer<int16_t>();
  reader.require_end();
  result.profile.pitch_step = result.profile.source[0];
  result.profile.bank_rate = result.profile.source[1];
  result.profile.forward_step = result.profile.source[2];
  result.profile.bank_acceleration = result.profile.source[4];
  result.profile.bank_turn_shift = result.profile.source[5];
  result.profile.maximum_bank = result.profile.source[7];
  result.models = read_model(resources.read("simvol0", "dts/" + std::string{name} + ".DTS"));
  result.textures = read_bitmaps(resources.read("simvol0", "dba/ENEMY.DBA"));
  result.damage = read_damage_definition(resources.read("simvol0", "dmg/" + std::string{name} + ".DMG"));
  result.collision = read_collision_parts(resources.read("simvol0", "col/" + std::string{name} + ".COL"));
  if(result.damage.hit_points.size() != 1 || result.damage.components.size() != 1) reader.fail("flyer requires the original single-section damage layout");
  return result;
}
} // namespace earthsiege
