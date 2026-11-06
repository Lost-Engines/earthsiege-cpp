#include "projectile_motion.h"

namespace earthsiege {

std::vector<beam_style> read_beam_styles(bytes const source) {
  binary_reader file{source,"beam styles"};
  std::vector<beam_style> result(file.integer<uint16_t>());
  for(auto &style : result) {
    style.width=file.integer<int16_t>();
    auto const colour{file.integer<int16_t>()};
    if(style.width<0 || colour<0 || colour>255) file.fail("invalid beam style");
    style.colour=static_cast<uint8_t>(colour);
  }
  file.require_end();return result;
}

std::vector<bullet_motion> read_bullet_motions(bytes const source, game_edition const edition) {
  binary_reader file{source, "bullet motions"};
  std::vector<bullet_motion> result(file.integer<uint16_t>());
  for(auto &motion : result) {
    motion.model = file.integer<int16_t>();
    motion.lifetime = file.integer<int16_t>();
    motion.collision_radius = file.integer<int16_t>();
    motion.animation_period = file.integer<int16_t>();
    motion.shot_noise = file.integer<int16_t>();
    if(edition == game_edition::expansion) {
      motion.spread = file.integer<int16_t>();
      motion.damage_falloff = file.integer<int16_t>();
    }
  }
  file.require_end();
  return result;
}

std::vector<rocket_motion> read_rocket_motions(bytes const source) {
  binary_reader file{source, "rocket motions"};
  std::vector<rocket_motion> result(file.integer<uint16_t>());
  for(auto &motion : result) {
    motion.model = file.integer<int16_t>();
    motion.lifetime = file.integer<int16_t>();
    motion.acceleration = file.integer<int16_t>();
    motion.collision_radius = file.integer<int16_t>();
    motion.animation_period = file.integer<int16_t>();
    motion.cell_sequence = file.integer<int16_t>();
    motion.shot_noise = file.integer<int16_t>();
  }
  file.require_end();
  return result;
}

} // namespace earthsiege
