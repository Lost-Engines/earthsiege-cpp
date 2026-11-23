#pragma once
#include <functional>
#include <string>
#include "render/instrument_bars.h"
#include "simulation/weapon_display.h"

namespace earthsiege {

/// Shared original weapon-row artwork and GAU geometry, indexed by cockpit control number.
class weapon_instruments {
  struct charge_pixels {
    bitmap painted;
    std::vector<uint8_t> saved_thumb;
    int thumb_boundary{0};
    int led_boundary{0};
    int32_t value{0};
    int32_t requested{0};
    bool full{true};
  };
  std::vector<charge_pixels> charge_cache;
  std::vector<std::optional<std::array<unsigned int, 5>>> header_cache;
  std::vector<bool> expanded_label;
  cockpit_layout layout;
  bool expansion;
  std::vector<bitmap> buttons;
  std::array<font, 3> number_fonts;
  font blue;
  font green;
  font yellow;
  std::string offline;
  std::string ecm;
  std::array<std::string, 5> group_labels;
  struct row_layout {
    rectangle button, number, lamp, name, offline, detail;
    slider_geometry charge_slider() const {
      return {detail, false, 1, 0, 1024};
    }
  };
  row_layout geometry(unsigned int slot, weapon_instrument_kind kind) const;
  void draw_row(framebuffer &target, row_layout const &row, unsigned int slot, weapon_instrument_kind kind,
                std::string_view name, weapon_instrument_state const &state,
                unsigned int button_face, int offset_x, int offset_y, bool expanded = false) const;
public:
  weapon_instruments(resource_store const &resources, cockpit_layout const &layout);
  std::array<rectangle, 5> group_bounds() const;
  void draw_groups(framebuffer &target, unsigned int group, std::array<unsigned int, 5> const &faces = {}, int offset_x = 0, int offset_y = 0) const;
  rectangle button_bounds(unsigned int slot, weapon_instrument_kind kind) const;
  slider_geometry charge_slider(unsigned int slot) const;
  /// 389ac/38b30 preserve the pixels under the moving charge thumb.
  void repaint();
  /// Authored rectangles actually overwritten by incremental painters.
  std::function<void(rectangle)> painted_region;
  void draw_retained(framebuffer &target, unsigned int slot, weapon_instrument_kind kind, std::string_view name,
                     weapon_instrument_state const &state, unsigned int button_face = 0,
                     std::optional<int> charge_boundary = {}, int offset_x = 0, int offset_y = 0);
  void draw(framebuffer &target, unsigned int slot, weapon_instrument_kind kind, std::string_view name,
            weapon_instrument_state const &state, unsigned int button_face = 0,
            std::optional<int> charge_boundary = {}, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
