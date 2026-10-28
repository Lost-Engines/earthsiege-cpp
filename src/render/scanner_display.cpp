#include "scanner_display.h"
#include <optional>
#include <stdexcept>
#include "resources/interface_strings.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
int32_t project_component(int32_t const distance, int16_t const coefficient) {
  int64_t const product{static_cast<int64_t>(distance) * coefficient};
  // 0x5e838 keeps IMUL's overflow flag, then adds a zero-extended low-word bias.
  uint32_t const bias{product != wrap_long(product) ? uint32_t{0xffffdfff} : uint32_t{0x2000}};
  return wrap_long((product + bias) >> matrix_fraction_bits);
}
std::string padded_distance(int32_t const distance) {
  auto text{std::to_string((distance / 1000) * 6)};
  if(text.size() < 4) text.insert(0, 4 - text.size(), '0');
  return text;
}
uint8_t contact_colour(scanner_contact const &contact) {
  if(contact.side == 0) return 50;
  switch(contact.kind) {
    case combat_actor_kind::herc: return 53;
    case combat_actor_kind::base: return 55;
    case combat_actor_kind::flyer: return 59;
  }
  return 62;
}
void label(framebuffer &target, font const &face, std::string_view const text, rectangle const &bounds, int const alignment) {
  int const x{alignment == 2 ? bounds.left + ((bounds.right - bounds.left) >> 1) - ((face.text_width(text) - 1) >> 1) :
              alignment == 4 ? bounds.right + 1 - (face.text_width(text) - 1) : bounds.left};
  int const y{bounds.top + ((bounds.bottom - bounds.top + static_cast<int>(face.height)) >> 1)};
  target.text(face, text, x, y);
}
}
scanner_display::scanner_display(resource_store const &resources)
  : background{read_bitmaps(resources.read("simvol0", "dba/RADAR.DBA")).at(0)},
    fonts{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN")),
          read_font(resources.read("simvol0", "dfn/CPGREEN.DFN")),
          read_font(resources.read("simvol0", "dfn/CPRED.DFN"))} {
  auto const strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))};
  activity_labels = {strings.at(25).at(0).text, strings.at(25).at(1).text};
  distance_labels = {strings.at(26).at(0).text, strings.at(27).at(0).text};
}
void scanner_display::draw(framebuffer &target, rectangle const &bounds, scanner_display_state const &state) const {
  scoped_framebuffer_clip clip{target, nullptr};
  int32_t const scale{state.range / 21};
  if(scale == 0) throw std::invalid_argument{"scanner range is too small"};
  int const x{bounds.left}, y{bounds.top};
  target.image(background, x, y, true);
  label(target, fonts[state.radar_active ? 1 : 2], activity_labels[state.radar_active ? 0 : 1], {x + 1, y + 2, x + 15, y + 7}, 2);
  std::optional<vec2<int32_t>> selected;
  int32_t selected_distance{};
  for(auto const &contact : state.contacts) {
    if(!contact.active) continue;
    auto position{contact.position};
    if(contact.side == 1 && !contact.detected) {
      if((state.ticks & 32) || contact.last_position_unavailable) continue;
      position = contact.last_position;
    }
    vec2<int32_t> const delta{wrap_long(static_cast<int64_t>(position.x) - state.player_position.x), wrap_long(static_cast<int64_t>(position.y) - state.player_position.y)};
    auto distance{fast_distance_2d(delta)};
    if(distance == 0) distance = 1;
    if(distance >= state.range) continue;
    if(delta.y > 0) distance = -distance;
    auto const sine{wrap_word(wrap_long(static_cast<int64_t>(delta.x) * model_matrix_one) / distance)};
    auto const angle{static_cast<uint16_t>(angle_quarter_turn - model_arcsine(sine) + state.heading)};
    vec2<int32_t> const point{x + 34 + project_component(distance, model_cosine(angle)) / scale,
                              y + 22 + project_component(distance, model_cosine(static_cast<uint16_t>(angle - angle_quarter_turn))) / scale};
    target.pixel(point.x, point.y, contact_colour(contact));
    if(contact.selected) { selected = point; selected_distance = distance < 0 ? -distance : distance; }
  }
  if(selected) {
    target.fill({selected->x - 2, selected->y - 2, selected->x + 2, selected->y - 2}, 59);
    target.fill({selected->x - 2, selected->y + 2, selected->x + 2, selected->y + 2}, 59);
    target.fill({selected->x - 2, selected->y - 2, selected->x - 2, selected->y + 2}, 59);
    target.fill({selected->x + 2, selected->y - 2, selected->x + 2, selected->y + 2}, 59);
  }
  label(target, fonts[0], distance_labels[1], {x + 54, y + 33, x + 66, y + 38}, 4);
  label(target, fonts[0], padded_distance(selected_distance), {x + 50, y + 38, x + 66, y + 43}, 4);
  label(target, fonts[0], distance_labels[0], {x + 3, y + 33, x + 15, y + 38}, 1);
  label(target, fonts[0], padded_distance(state.range), {x + 3, y + 38, x + 19, y + 43}, 1);
}
} // namespace earthsiege
