#include "effects.h"
#include "simulation/fixed_math.h"

namespace earthsiege {

uint8_t explosion_definition::frame_light(uint16_t const frame) const {
  // Definitions 16/17 have a thirteenth frame. The original lookup aliases the
  // following distance word, making that final frame intensity 20000 & 255 = 32.
  if(frame == light_curve.size()) return static_cast<uint8_t>(light_range);
  return static_cast<uint8_t>(light_curve.at(frame));
}

std::vector<explosion_definition> read_explosions(bytes const source) {
  binary_reader file{source, "explosion definitions"};
  std::vector<explosion_definition> definitions(file.integer<uint16_t>());
  for(auto &entry : definitions) {
    entry.model = file.integer<int16_t>();
    entry.cell_sequence = file.integer<int16_t>();
    entry.texture_bank = file.integer<int16_t>();
    entry.animation_period = file.integer<int16_t>();
    entry.secondary = file.integer<int16_t>() != 0;
    entry.light = static_cast<effect_light_kind>(file.integer<int16_t>());
    if(entry.light < effect_light_kind::none || entry.light > effect_light_kind::distance_gated) file.fail("unknown explosion light kind");
    for(auto &value : entry.light_curve) value = file.integer<int16_t>();
    entry.light_range = file.integer<int32_t>();
    entry.sound = file.integer<int16_t>();
    entry.alternate_object_kind = file.integer<int16_t>() != 0;
  }
  file.require_end();
  return definitions;
}

std::vector<debris_definition> read_debris(bytes const source) {
  binary_reader file{source, "debris definitions"};
  std::vector<debris_definition> definitions(file.integer<uint16_t>());
  for(auto &entry : definitions) {
    entry.count_limit = file.integer<int16_t>();
    entry.models.resize(file.integer<uint16_t>());
    for(auto &model : entry.models) {
      model = {file.integer<int16_t>(), file.integer<int16_t>()};
      if(model.model < 0 || model.weight < 0) file.fail("invalid debris model/weight");
      entry.total_weight = wrap_word(entry.total_weight + model.weight);
    }
    if(entry.count_limit <= 0 || entry.total_weight <= 0) file.fail("invalid debris choice range");
  }
  file.require_end();
  return definitions;
}

} // namespace earthsiege
