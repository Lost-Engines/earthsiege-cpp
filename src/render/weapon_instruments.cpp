#include "weapon_instruments.h"
#include <algorithm>
#include <stdexcept>
#include "resources/interface_strings.h"

namespace earthsiege {
namespace {
rectangle translate(rectangle value, int const x, int const y) {
  value.left += x;
  value.right += x;
  value.top += y;
  value.bottom += y;
  return value;
}
struct weapon_template_offsets {
  size_t bounds, mirror_mask;
};
weapon_template_offsets template_offsets(weapon_instrument_kind const kind) {
  switch(kind) {
  case weapon_instrument_kind::energy:
    return {0xf4, 0x134};
  case weapon_instrument_kind::ammunition:
    return {0xa4, 0xd4};
  case weapon_instrument_kind::ecm:
    return {0x154, 0x1a4};
  }
  throw std::runtime_error{"unknown weapon instrument"};
}
void label(framebuffer &target, font const &face, std::string_view text, rectangle const &bounds,
           bool const centred, int const x_offset, uint8_t const background = 0) {
  int const x{centred ? bounds.left + ((bounds.right - bounds.left) >> 1) - ((face.text_width(text) - 1) >> 1) : bounds.left + x_offset};
  int const y{bounds.top + ((bounds.bottom - bounds.top + static_cast<int>(face.height)) >> 1)};
  if(background) target.fill(bounds, background);
  target.text(face, text, x, y);
}
} // anonymous namespace

weapon_instruments::weapon_instruments(resource_store const &resources, cockpit_layout const &source)
  : charge_cache(source.weapon_count()),
    header_cache(source.weapon_count()),
    expanded_label(source.weapon_count()),
    layout{source},
    expansion{resources.edition() == game_edition::expansion},
    buttons{source.read_buttons(resources)},
    number_fonts{read_font(resources.read("simvol0", "dfn/CPON.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPPRESS.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPOFF.DFN"))},
    blue{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN"))},
    green{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN"))},
    yellow{read_font(resources.read("simvol0", "dfn/CPYLW.DFN"))} {
  auto const strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))};
  offline = strings.at(2).at(0).text;
  ecm = strings.at(3).at(0).text;
  for(size_t i{0}; i < group_labels.size(); ++i) group_labels[i] = strings.at(4).at(i).text;
}

std::array<rectangle, 5> weapon_instruments::group_bounds() const {
  auto const parent{layout.bounds(0x1c4)};
  std::array<rectangle, 5> result;
  for(size_t i{0}; i < result.size(); ++i) result[i] = translate(layout.bounds(0x1d4 + i * 16), parent.left, parent.top);
  return result;
}
void weapon_instruments::draw_groups(framebuffer &target, unsigned int const group, std::array<unsigned int, 5> const &faces, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const bounds{group_bounds()};
  for(size_t i{0}; i < bounds.size(); ++i) {
    auto const area{translate(bounds[i], offset_x, offset_y)};
    if(i < 3) {
      label(target, i == group ? yellow : blue, group_labels[i], area, true, 0, 63);
    } else {
      target.image(buttons.at(6 + faces[i]), area.left, area.top, true);
      label(target, number_fonts[0], group_labels[i], area, true, 0);
    }
  }
}

weapon_instruments::row_layout weapon_instruments::geometry(unsigned int const slot, weapon_instrument_kind const kind) const {
  if(slot >= layout.weapon_count()) throw std::out_of_range{"weapon cockpit control number"};
  auto const parent{layout.bounds(0x14 + slot * 16)};
  auto const offsets{template_offsets(kind)};
  // Expansion DBSIM gives each weapon template a byte mask, one bit per row.
  bool const mirrored{expansion && (static_cast<uint8_t>(layout.words[offsets.mirror_mask / 4]) & (1u << slot))};
  auto const child{[&](size_t offset) {
    auto const local{layout.bounds(offsets.bounds + offset)};
    if(mirrored) return rectangle{parent.right - local.right, parent.top + local.top, parent.right - local.left, parent.top + local.bottom};
    return translate(local, parent.left, parent.top);
  }};
  auto const header{child(0x10)};
  auto number{header};
  int const number_width{expansion ? 9 : 10};
  if(mirrored) number.left = header.right - number_width + 1;
  else number.right = header.left + number_width - 1;
  rectangle const button{number.left, number.top, number.left + 8, number.top + static_cast<int>(buttons.at(0).height)};
  int const lamp_x{mirrored ? number.left - 4 : number.right + 2};
  rectangle const lamp{lamp_x, header.top, lamp_x + 2, header.top + 7};
  auto name{header};
  if(mirrored) name.right = lamp.left - 2;
  else name.left = lamp.right + 2;
  auto offline{parent};
  if(mirrored) {
    offline.right = name.right;
    offline.bottom = name.bottom;
  } else {
    offline.left = expansion ? name.left : button.left;
    offline.top = header.top;
  }
  return {button, number, lamp, name, offline, child(0x20)};
}
rectangle weapon_instruments::button_bounds(unsigned int const slot, weapon_instrument_kind const kind) const {
  return geometry(slot, kind).button;
}
slider_geometry weapon_instruments::charge_slider(unsigned int const slot) const {
  return geometry(slot, weapon_instrument_kind::energy).charge_slider();
}

void weapon_instruments::draw_row(framebuffer &target, row_layout const &row, unsigned int const slot, weapon_instrument_kind const kind,
                                  std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                                  int const offset_x, int const offset_y, bool const expanded) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const button{translate(row.button, offset_x, offset_y)};
  if(state.disabled && kind != weapon_instrument_kind::ecm) button_face = 2;
  target.image(buttons.at(button_face), button.left, button.top, true);
  label(target, number_fonts.at(button_face), std::to_string(slot + 1), translate(row.number, offset_x, offset_y), true, 0);
  auto const detail{translate(row.detail, offset_x, offset_y)};
  if(kind == weapon_instrument_kind::ecm) {
    auto bounds{detail};
    if(state.disabled) {
      bounds.left = button.left;
      bounds.top = button.top;
    }
    label(target, yellow, state.disabled ? offline : ecm, bounds, true, 0, state.selected && !state.disabled ? 56 : 63);
    return;
  }
  auto const lamp{translate(row.lamp, offset_x, offset_y)};
  auto const name_bounds{translate(state.disabled || expanded ? row.offline : row.name, offset_x, offset_y)};
  label(target, state.disabled || state.selected ? yellow : green, state.disabled ? offline : name,
        name_bounds, state.disabled || expanded, 1, !state.disabled && state.selected ? 53 : 63);
  if(state.disabled || state.selected || state.included) {
    target.image(buttons.at(state.disabled ? 57 : state.ready ? 55 : 56), lamp.left, lamp.top, true);
  } else {
    target.fill(lamp, 63);
  }
  if(state.disabled) return;
  if(kind == weapon_instrument_kind::ammunition) {
    label(target, yellow, std::to_string(state.value), detail, false, 0, 63);
    return;
  }
}

void weapon_instruments::draw(framebuffer &target, unsigned int const slot, weapon_instrument_kind const kind,
                              std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                              std::optional<int> const charge_boundary, int const offset_x, int const offset_y) const {
  auto const row{geometry(slot, kind)};
  draw_row(target, row, slot, kind, name, state, button_face, offset_x, offset_y);
  if(kind != weapon_instrument_kind::energy || state.disabled) return;
  scoped_framebuffer_clip clip{target, nullptr};
  auto const detail{translate(row.detail, offset_x, offset_y)};
  // 0x389ac resets the LED boundary, then paints only the charged interval.
  // Uncharged pixels belong to the cockpit backing, not to a full bar clear.
  int const extent{(static_cast<int16_t>(state.value) * ((detail.right - detail.left) * 65536 / 1024)) >> 16};
  if(extent > 0) {
    for(int x{detail.left}; x <= detail.left + extent; ++x) {
      target.fill({x, detail.top, x, detail.bottom}, x & 1 ? 49 : 50);
    }
  } else if(extent < 0) target.fill({detail.left + extent + 1, detail.top, detail.left, detail.bottom}, 63);
  int const boundary{charge_boundary.value_or(row.charge_slider().boundary_for_value(state.requested_charge))};
  target.image(buttons.at(26), boundary + offset_x, detail.top - 1, true);
}

void weapon_instruments::repaint() {
  for(auto &charge : charge_cache) charge.full = true;
  for(auto &header : header_cache) header.reset();
}
void weapon_instruments::draw_retained(framebuffer &target, unsigned int const slot, weapon_instrument_kind const kind,
                                       std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                                       std::optional<int> const charge_boundary, int const offset_x, int const offset_y) {
  // 37194/376b4 mark the header only for enable/selection/group/ready
  // or button-face changes; ammunition and charge have separate dirty flags.
  std::array<unsigned int, 5> const header{state.disabled, state.selected, state.included, state.ready, button_face};
  auto const row{geometry(slot, kind)};
  auto &previous{header_cache.at(slot)};
  bool header_changed{false};
  if(!previous || *previous != header) {
    auto const shown{[&](rectangle const &b){
      return b.right + offset_x >= 0 && b.left + offset_x < 320 && b.bottom + offset_y >= 0 && b.top + offset_y < 200;
    }};
    auto const &button{row.button};
    if(shown(button)) {
      header_changed = true;
      // 384f0/38509 enlarge and centre the label for OFFLINE. The original
      // never restores its old bounds when that weapon becomes operational.
      if(state.disabled && kind != weapon_instrument_kind::ecm) expanded_label[slot] = true;
      if(painted_region) {
        painted_region(button);
        if(expanded_label[slot]) painted_region(row.offline);
        if(kind == weapon_instrument_kind::ecm) {
          auto detail{row.detail};
          if(state.disabled) {
            detail.left = button.left;
            detail.top = button.top;
          }
          if(shown(detail)) painted_region(detail);
        } else {
          if(shown(row.lamp)) painted_region(row.lamp);
          if(shown(row.name)) painted_region(row.name);
        }
      }
      previous = header;
    }
  }
  draw_row(target, row, slot, kind, name, state, button_face, offset_x, offset_y, expanded_label[slot]);
  if(kind != weapon_instrument_kind::energy || state.disabled) return;
  scoped_framebuffer_clip clip{target, nullptr};
  auto const bounds{row.detail};
  auto const area{translate({bounds.left, bounds.top - 1, bounds.right, bounds.bottom + 1}, offset_x, offset_y)};
  // Original 2b214 does not advance controls outside the current cockpit page.
  if(area.right < 0 || area.bottom < 0 || area.left >= static_cast<int>(screen_width) || area.top >= static_cast<int>(screen_height)) return;
  auto &cache{charge_cache.at(slot)};
  auto const &thumb{buttons.at(26)};
  if(cache.painted.pixels.empty()) {
    cache.painted.width = static_cast<unsigned>(bounds.right - bounds.left + 1);
    cache.painted.height = static_cast<unsigned>(bounds.bottom - bounds.top + 3);
    cache.painted.pixels.resize(cache.painted.width * cache.painted.height);
    cache.saved_thumb.resize(2 * thumb.height);                                // fresh DOS allocation, before its first save
    cache.thumb_boundary = bounds.left;
    cache.led_boundary = bounds.left;
  }
  auto &image{cache.painted};
  auto pixel{[&](int x, int y, uint8_t colour){
    if(x >= 0 && y >= 0 && x < static_cast<int>(image.width) && y < static_cast<int>(image.height)) {
      image.pixels[y * image.width + x] = colour;
    }
  }};
  // A retained OFFLINE label overlaps the slider too. Its later repaint
  // changes the live backing before the slider restores its saved thumb.
  if(header_changed && expanded_label[slot] && !cache.full) {
    auto const label_bounds{translate(layout.bounds(0x14 + slot * 16), offset_x, offset_y)};
    for(unsigned y{0}; y < image.height; ++y) {
      for(unsigned x{0}; x < image.width; ++x) {
        int const sx{area.left + static_cast<int>(x)};
        int const sy{area.top + static_cast<int>(y)};
        if(sx >= 0 && sx < 320 && sy >= 0 && sy < 200 && sx >= label_bounds.left &&
           sx <= label_bounds.right && sy >= label_bounds.top && sy <= label_bounds.bottom) {
          image.pixels[y * image.width + x] = target.pixels()[sy * screen_width + sx];
        }
      }
    }
  }
  if(cache.full) {
    for(unsigned y{0}; y < image.height; ++y) {
      for(unsigned x{0}; x < image.width; ++x) {
        int const sx{area.left + static_cast<int>(x)};
        int const sy{area.top + static_cast<int>(y)};
        if(sx >= 0 && sy >= 0 && sx < static_cast<int>(screen_width) && sy < static_cast<int>(screen_height)) {
          image.pixels[y * image.width + x] = target.pixels()[sy * screen_width + sx];
        }
      }
    }
  }
  auto const boundary{charge_boundary.value_or(row.charge_slider().boundary_for_value(state.requested_charge))};
  if(cache.full || cache.value != state.value || cache.requested != state.requested_charge || cache.thumb_boundary != boundary) {
    // Full paint restores only a displaced thumb; incremental paint always
    // restores it, even when only the LED value changed.
    if(!cache.full || cache.thumb_boundary != boundary) {
      if(painted_region) painted_region({cache.thumb_boundary, bounds.top - 1, cache.thumb_boundary + 1, bounds.top + static_cast<int>(thumb.height) - 2});
      for(unsigned y{0}; y < thumb.height; ++y) {
        for(unsigned x{0}; x < 2; ++x) {
          pixel(cache.thumb_boundary - bounds.left + static_cast<int>(x), static_cast<int>(y), cache.saved_thumb[y * 2 + x]);
        }
      }
    }
    if(cache.full) cache.led_boundary = bounds.left;
    if(cache.full || cache.value != state.value) {
      int const next{bounds.left + ((static_cast<int16_t>(state.value) * ((bounds.right - bounds.left) * 65536 / 1024)) >> 16)};
      // 31e5c includes the old endpoint when charging, but preserves the new
      // endpoint when discharging. Resetting the boundary does not clear pixels.
      int const first{next > cache.led_boundary ? cache.led_boundary : next + 1};
      int const last{next > cache.led_boundary ? next : cache.led_boundary};
      if(next != cache.led_boundary) {
        if(painted_region) painted_region({first, bounds.top, last, bounds.bottom});
        for(int x{first}; x <= last; ++x) {
          for(int y{1}; y < static_cast<int>(image.height) - 1; ++y) {
            pixel(x - bounds.left, y, next > cache.led_boundary ? (x & 1 ? 49 : 50) : 63);
          }
        }
      }
      cache.led_boundary = next;
    }
    for(unsigned y{0}; y < thumb.height; ++y) {
      for(unsigned x{0}; x < 2; ++x) {
        auto const ix{boundary - bounds.left + static_cast<int>(x)};
        if(ix >= 0 && ix < static_cast<int>(image.width) && y < image.height) {
          cache.saved_thumb[y * 2 + x] = image.pixels[y * image.width + ix];
        }
      }
    }
    for(unsigned y{0}; y < thumb.height; ++y) {
      for(unsigned x{0}; x < thumb.width; ++x) {
        if(auto const colour{thumb.pixels[y * thumb.width + x]}) {
          pixel(boundary - bounds.left + static_cast<int>(x), static_cast<int>(y), colour);
          int const tx{boundary + static_cast<int>(x)};
          int const ty{bounds.top - 1 + static_cast<int>(y)};
          if(painted_region) painted_region({tx, ty, tx, ty});
        }
      }
    }
    cache.thumb_boundary = boundary;
    cache.value = state.value;
    cache.requested = state.requested_charge;
    cache.full = false;
  }
  target.image(image, area.left, area.top);
}

} // namespace earthsiege
