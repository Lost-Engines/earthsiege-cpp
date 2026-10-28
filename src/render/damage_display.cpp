#include "damage_display.h"
#include <algorithm>
#include <stdexcept>
#include "resources/name_table.h"

namespace earthsiege {
namespace {
int health_percent(int const damage) { return (256 - damage) * 100 >> 8; }
size_t health_grade(int const damage) {
  int const health{health_percent(damage)};
  return health >= 90 ? 0U : health >= 74 ? 1U : health >= 51 ? 2U : health >= 1 ? 3U : 4U;
}
int region_damage(int32_t const section, std::array<int16_t, 19> const &armour) {
  switch(section) {
    case 0: return (armour[0] + armour[1]) / 2;
    case 4: case 5: case 6: return armour[static_cast<size_t>(section)];
    case 7: case 8: case 13: case 14: {
      auto const i{static_cast<size_t>(section)};
      return (armour[i] + armour[i + 2] + armour[i + 4]) / 3;
    }
    default: throw std::runtime_error{"unknown cockpit damage section"};
  }
}
int region_colour(paper_doll_region const &region, int const damage) {
  auto const grade{health_grade(damage)};
  if(region.colour_mode == damage_colour_mode::exact_colour) {
    if(grade == 3) return 48;
    if(grade == 4) return 62;
    return region.base_colour + (region.base_colour == 60 ? 1 : -1) * static_cast<int>(grade);
  }
  return std::array{54, 57, 92, 51, 62}[grade];
}
void remap(framebuffer &target, paper_doll_region const &region, int const colour, int const x, int const y) {
  auto bounds{region.bounds}; bounds.left += x; bounds.right += x; bounds.top += y; bounds.bottom += y;
  for(int row{std::max(0, bounds.top)}; row <= std::min(static_cast<int>(screen_height) - 1, bounds.bottom); ++row) {
    for(int column{std::max(0, bounds.left)}; column <= std::min(static_cast<int>(screen_width) - 1, bounds.right); ++column) {
      int const pixel{target.pixels()[static_cast<size_t>(row) * screen_width + static_cast<size_t>(column)]};
      int replacement{pixel};
      switch(region.colour_mode) {
        case damage_colour_mode::shaded_range:
          if(pixel >= region.base_colour && pixel <= region.base_colour + 2) replacement += colour - region.base_colour;
          break;
        case damage_colour_mode::exact_colour:
          if(pixel == region.base_colour) replacement = colour;
          break;
        case damage_colour_mode::additive:
          if(pixel != 63) replacement += colour;
          break;
        case damage_colour_mode::solid_range:
          if(pixel >= region.base_colour && pixel <= region.base_colour + 2) replacement = colour;
          break;
      }
      target.pixel(column, row, static_cast<uint8_t>(replacement));
    }
  }
}
size_t herc_status(damage_display_state const &state) {
  if(state.dead) return 4;
  auto const [least, most]{std::minmax_element(state.internal.begin(), state.internal.end())};
  if(*least > 128) return 3;
  if(*most > 0) return 2;
  return state.shield_warning ? 1U : 0U;
}
}

damage_display::damage_display(resource_store const &resources)
  : bases{read_bitmaps(resources.read("simvol0", "dba/BASES.DBA"))},
    flyers{read_bitmaps(resources.read("simvol0", "dba/FLYERS.DBA"))},
    fonts{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN")),
          read_font(resources.read("simvol0", "dfn/CPGREEN.DFN")),
          read_font(resources.read("simvol0", "dfn/CPRED.DFN"))},
    strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))} {
  for(auto const &name : read_resource_names(resources.read("simvol0", "nam/MECHS.NAM")))
    chassis.push_back({read_paper_doll(resources.read("simvol0", "pdg/" + name + ".PDG")),
                       read_bitmaps(resources.read("simvol0", "dba/" + name + ".DBA"))});
}

void damage_display::draw(framebuffer &target, rectangle const &bounds, damage_display_state const &state) {
  scoped_framebuffer_clip clip{target, nullptr};
  target.fill(bounds, 63);
  auto const label{[&](size_t const index, std::string_view const text, size_t const font_index = 2) {
    auto const &face{fonts.at(font_index)};
    int const top{bounds.top + std::array{0, 8, 21, 28, 35}.at(index)};
    int const x{bounds.left + 45 - ((face.text_width(text) - 1) >> 1)};
    int const y{top + ((10 + static_cast<int>(face.height)) >> 1)};
    target.text(face, text, x, y);
  }};
  auto const text{[&](size_t const group, size_t const item = 0)->std::string const & { return strings.at(group).at(item).text; }};
  auto const status{[&](size_t const grade) {
    label(3, text(24, grade));
    if(state.friendly) label(4, "[ " + std::to_string(health_percent(state.total_damage)) + "% ]");
  }};
  switch(state.kind) {
    case damage_display_kind::none:
    case damage_display_kind::unknown:
      name_font = 0; label(0, text(17)); label(1, text(state.kind == damage_display_kind::none ? 22 : 23), name_font);
      break;
    case damage_display_kind::herc: {
      auto const &art{chassis.at(static_cast<size_t>(state.model))};
      auto const &view{art.diagram.views.at(2)};
      int const x{bounds.left + view.bounds.left}, y{bounds.top + view.bounds.top};
      target.image(art.frames.at(2), x, y, true);
      if(!state.player) {
        name_font = state.friendly ? 1 : 2;
        label(0, text(17)); label(1, text(19, static_cast<size_t>(state.model)), name_font);
      }
      label(2, text(18));
      for(auto const &region : view.regions) remap(target, region, region_colour(region, region_damage(region.section, state.armour)), x, y);
      status(herc_status(state));
      break;
    }
    case damage_display_kind::ground:
    case damage_display_kind::flyer: {
      auto const &image{state.kind == damage_display_kind::ground ? bases.at(static_cast<size_t>(state.model)) : flyers.at(0)};
      target.image(image, bounds.left + 2, bounds.bottom - static_cast<int>(image.height) - 5, true);
      label(0, text(17)); label(1, state.kind == damage_display_kind::ground ? text(20, static_cast<size_t>(state.model)) : text(21), name_font);
      status(health_grade(state.total_damage));
      break;
    }
  }
}
} // namespace earthsiege
