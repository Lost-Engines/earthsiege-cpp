#pragma once
#include <array>
#include "render/framebuffer.h"
#include "resources/binary_reader.h"

namespace earthsiege {

enum class damage_colour_mode : int32_t { shaded_range, exact_colour, additive, solid_range };
struct paper_doll_region {
  int32_t section;
  rectangle bounds;
  int32_t base_colour;
  damage_colour_mode colour_mode;
};
struct paper_doll_view {
  rectangle bounds;
  std::vector<paper_doll_region> regions;
};
/// PDG regions refer to the corresponding DBA frame; mounts retain their stored words.
struct paper_doll {
  std::vector<paper_doll_view> views;
  std::vector<std::array<int32_t, 5>> mounts;
};
paper_doll read_paper_doll(bytes source);
std::vector<std::array<int32_t, 2>> read_weapon_icon_sizes(bytes source);

} // namespace earthsiege
