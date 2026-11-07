#pragma once
#include "resources/artwork.h"
#include "resources/interface_strings.h"
#include "resources/paper_doll.h"
#include "resources/resource_store.h"

namespace earthsiege {

enum class damage_display_kind : int16_t { none = -1, herc, ground, flyer, unknown };
struct damage_display_state {
  damage_display_kind kind{damage_display_kind::none};
  int16_t model{};
  bool friendly{};
  bool player{};
  bool dead{};
  bool shield_warning{};
  int16_t total_damage{};
  std::array<int16_t, 19> armour{};
  std::array<int16_t, 12> internal{};
  std::string name;
};

/// STATUS/TARGET share an original paper doll renderer and retain the name font.
class damage_display {
  struct chassis_artwork { paper_doll diagram; std::vector<bitmap> frames; };
  std::vector<chassis_artwork> chassis;
  std::vector<bitmap> bases;
  std::vector<bitmap> flyers;
  std::array<font, 3> fonts;                                                 // blue, green, red
  interface_strings strings;
  size_t name_font{2};
public:
  explicit damage_display(resource_store const &resources);
  void draw(framebuffer &target, rectangle const &bounds, damage_display_state const &state);
};

} // namespace earthsiege
