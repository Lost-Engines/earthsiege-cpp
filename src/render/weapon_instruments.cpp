#include "weapon_instruments.h"
#include <algorithm>
#include <stdexcept>
#include "resources/interface_strings.h"

namespace earthsiege {
namespace {
rectangle translate(rectangle value, int const x, int const y) {
  value.left += x; value.right += x; value.top += y; value.bottom += y;
  return value;
}
size_t template_offset(weapon_instrument_kind const kind) {
  switch(kind) {
    case weapon_instrument_kind::energy: return 0xf4;
    case weapon_instrument_kind::ammunition: return 0xa4;
    case weapon_instrument_kind::ecm: return 0x154;
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
} // namespace

weapon_instruments::weapon_instruments(resource_store const &resources, cockpit_layout const &source)
  : charge_cache(source.weapon_count()), layout{source}, buttons{read_bitmaps(resources.read("simvol0", "dba/PBUTTONS.DBA"))},
    number_fonts{read_font(resources.read("simvol0", "dfn/CPON.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPPRESS.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPOFF.DFN"))},
    blue{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN"))},
    green{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN"))},
    yellow{read_font(resources.read("simvol0", "dfn/CPYLW.DFN"))} {
  auto const strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))};
  offline = strings.at(2).at(0).text;
  ecm = strings.at(3).at(0).text;
  for(size_t i{}; i < group_labels.size(); ++i) group_labels[i] = strings.at(4).at(i).text;
}

std::array<rectangle, 5> weapon_instruments::group_bounds() const {
  auto const parent{layout.bounds(0x1c4)};
  std::array<rectangle, 5> result;
  for(size_t i{}; i < result.size(); ++i) result[i] = translate(layout.bounds(0x1d4 + i * 16), parent.left, parent.top);
  return result;
}
void weapon_instruments::draw_groups(framebuffer &target, unsigned int const group, std::array<unsigned int, 5> const &faces, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const bounds{group_bounds()};
  for(size_t i{}; i < bounds.size(); ++i) {
    auto const area{translate(bounds[i], offset_x, offset_y)};
    if(i < 3) label(target, i == group ? yellow : blue, group_labels[i], area, true, 0, 63);
    else {
      target.image(buttons.at(6 + faces[i]), area.left, area.top, true);
      label(target, number_fonts[0], group_labels[i], area, true, 0);
    }
  }
}

rectangle weapon_instruments::child_bounds(unsigned int const slot, weapon_instrument_kind const kind, size_t const offset) const {
  if(slot >= layout.weapon_count()) throw std::out_of_range{"weapon cockpit control number"};
  auto const parent{layout.bounds(0x14 + slot * 16)};
  return translate(layout.bounds(template_offset(kind) + offset), parent.left, parent.top);
}
rectangle weapon_instruments::button_bounds(unsigned int const slot, weapon_instrument_kind const kind) const {
  auto bounds{child_bounds(slot, kind, 0x10)};
  bounds.right = bounds.left + 8;
  bounds.bottom = bounds.top + static_cast<int>(buttons.at(0).height);
  return bounds;
}
slider_geometry weapon_instruments::charge_slider(unsigned int const slot) const {
  return {child_bounds(slot, weapon_instrument_kind::energy, 0x20), false, 1, 0, 1024};
}

void weapon_instruments::draw_row(framebuffer &target, unsigned int const slot, weapon_instrument_kind const kind,
                                  std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                                  int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const button{translate(child_bounds(slot, kind, 0x10), offset_x, offset_y)};
  if(state.disabled && kind != weapon_instrument_kind::ecm) button_face = 2;
  target.image(buttons.at(button_face), button.left, button.top, true);
  auto number{button}; number.right = number.left + 9;
  label(target, number_fonts.at(button_face), std::to_string(slot + 1), number, true, 0);
  auto const detail{translate(child_bounds(slot, kind, 0x20), offset_x, offset_y)};
  if(kind == weapon_instrument_kind::ecm) {
    auto bounds{detail};
    if(state.disabled) { bounds.left = button.left; bounds.top = button.top; }
    label(target, yellow, state.disabled ? offline : ecm, bounds, true, 0, state.selected && !state.disabled ? 56 : 63);
    return;
  }
  int const status_x{button.left + 11}, status_y{button.top};
  auto name_bounds{button}; name_bounds.left = status_x + 4;
  if(state.disabled) {
    name_bounds = translate(layout.bounds(0x14 + slot * 16), offset_x, offset_y);
    name_bounds.left = button.left; name_bounds.top = button.top;
  }
  label(target, state.disabled || state.selected ? yellow : green, state.disabled ? offline : name,
        name_bounds, state.disabled, 1, !state.disabled && state.selected ? 53 : 63);
  if(state.disabled || state.selected || state.included)
    target.image(buttons.at(state.disabled ? 57 : state.ready ? 55 : 56), status_x, status_y, true);
  else target.fill({status_x, status_y, status_x + 2, status_y + 7}, 63);
  if(state.disabled) return;
  if(kind == weapon_instrument_kind::ammunition) {
    label(target, yellow, std::to_string(state.value), detail, false, 0, 63);
    return;
  }
}

void weapon_instruments::draw(framebuffer &target, unsigned int const slot, weapon_instrument_kind const kind,
                              std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                              std::optional<int> const charge_boundary, int const offset_x, int const offset_y) const {
  draw_row(target,slot,kind,name,state,button_face,offset_x,offset_y);
  if(kind!=weapon_instrument_kind::energy || state.disabled)return;
  scoped_framebuffer_clip clip{target,nullptr};
  auto const detail{translate(child_bounds(slot,kind,0x20),offset_x,offset_y)};
  // 0x389ac resets the LED boundary, then paints only the charged interval.
  // Uncharged pixels belong to the cockpit backing, not to a full bar clear.
  int const extent{(static_cast<int16_t>(state.value) * ((detail.right - detail.left) * 65536 / 1024)) >> 16};
  if(extent > 0) for(int x{detail.left}; x <= detail.left + extent; ++x)
    target.fill({x, detail.top, x, detail.bottom}, x & 1 ? 49 : 50);
  else if(extent < 0) target.fill({detail.left + extent + 1, detail.top, detail.left, detail.bottom}, 63);
  auto const slider{charge_slider(slot)};
  int const boundary{charge_boundary.value_or(slider.boundary_for_value(state.requested_charge))};
  target.image(buttons.at(26), boundary + offset_x, detail.top - 1, true);
}

void weapon_instruments::repaint() {
  for(auto &charge:charge_cache)charge.full=true;
}
void weapon_instruments::draw_retained(framebuffer &target, unsigned int const slot, weapon_instrument_kind const kind,
                                       std::string_view name, weapon_instrument_state const &state, unsigned int button_face,
                                       std::optional<int> const charge_boundary, int const offset_x, int const offset_y) {
  draw_row(target,slot,kind,name,state,button_face,offset_x,offset_y);
  if(kind!=weapon_instrument_kind::energy || state.disabled)return;
  scoped_framebuffer_clip clip{target,nullptr};
  auto const bounds{child_bounds(slot,kind,0x20)};
  auto const area{translate({bounds.left,bounds.top-1,bounds.right,bounds.bottom+1},offset_x,offset_y)};
  // Original 2b214 does not advance controls outside the current cockpit page.
  if(area.right<0 || area.bottom<0 || area.left>=static_cast<int>(screen_width) || area.top>=static_cast<int>(screen_height))return;
  auto &cache{charge_cache.at(slot)};
  auto const &thumb{buttons.at(26)};
  if(cache.painted.pixels.empty()) {
    cache.painted.width=static_cast<unsigned>(bounds.right-bounds.left+1);
    cache.painted.height=static_cast<unsigned>(bounds.bottom-bounds.top+3);
    cache.painted.pixels.resize(cache.painted.width*cache.painted.height);
    cache.saved_thumb.resize(2*thumb.height); // fresh DOS allocation, before its first save
    cache.thumb_boundary=bounds.left;
    cache.led_boundary=bounds.left;
  }
  auto &image{cache.painted};
  auto pixel=[&](int x,int y,uint8_t colour) {
    if(x>=0&&y>=0&&x<static_cast<int>(image.width)&&y<static_cast<int>(image.height))
      image.pixels[y*image.width+x]=colour;
  };
  if(cache.full) {
    for(unsigned y{};y<image.height;++y)for(unsigned x{};x<image.width;++x) {
      int const sx{area.left+static_cast<int>(x)},sy{area.top+static_cast<int>(y)};
      if(sx>=0&&sy>=0&&sx<static_cast<int>(screen_width)&&sy<static_cast<int>(screen_height))
        image.pixels[y*image.width+x]=target.pixels()[sy*screen_width+sx];
    }
  }
  auto const boundary{charge_boundary.value_or(charge_slider(slot).boundary_for_value(state.requested_charge))};
  if(cache.full || cache.value!=state.value || cache.requested!=state.requested_charge || cache.thumb_boundary!=boundary) {
    // Full paint restores only a displaced thumb; incremental paint always
    // restores it, even when only the LED value changed.
    if(!cache.full || cache.thumb_boundary!=boundary)
      for(unsigned y{};y<thumb.height;++y)for(unsigned x{};x<2;++x)
        pixel(cache.thumb_boundary-bounds.left+static_cast<int>(x),static_cast<int>(y),cache.saved_thumb[y*2+x]);
    if(cache.full)cache.led_boundary=bounds.left;
    if(cache.full || cache.value!=state.value) {
      int const next{bounds.left+((static_cast<int16_t>(state.value)*((bounds.right-bounds.left)*65536/1024))>>16)};
      // 31e5c includes the old endpoint when charging, but preserves the new
      // endpoint when discharging. Resetting the boundary does not clear pixels.
      int const first{next>cache.led_boundary?cache.led_boundary:next+1};
      int const last{next>cache.led_boundary?next:cache.led_boundary};
      if(next!=cache.led_boundary)
        for(int x{first};x<=last;++x)for(int y{1};y<static_cast<int>(image.height)-1;++y)
          pixel(x-bounds.left,y,next>cache.led_boundary?(x&1?49:50):63);
      cache.led_boundary=next;
    }
    for(unsigned y{};y<thumb.height;++y)for(unsigned x{};x<2;++x) {
      auto const ix{boundary-bounds.left+static_cast<int>(x)};
      if(ix>=0&&ix<static_cast<int>(image.width)&&y<image.height)
        cache.saved_thumb[y*2+x]=image.pixels[y*image.width+ix];
    }
    for(unsigned y{};y<thumb.height;++y)for(unsigned x{};x<thumb.width;++x)
      if(auto const colour{thumb.pixels[y*thumb.width+x]})
        pixel(boundary-bounds.left+static_cast<int>(x),static_cast<int>(y),colour);
    cache.thumb_boundary=boundary;cache.value=state.value;cache.requested=state.requested_charge;cache.full=false;
  }
  target.image(image,area.left,area.top);
}

} // namespace earthsiege
