#include "clock_instrument.h"

namespace earthsiege {
clock_instrument::clock_instrument(resource_store const &resources, cockpit_layout const &layout, uint32_t const now, bool const initial_colon)
  : bounds{layout.bounds(0x654)}, face{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN"))}, deadline{now + 30}, colon{initial_colon} {}

void clock_instrument::refresh() {
  colon = !colon;
  // Original 0x43658 wraps the two minute digits at 25, including on a full
  // cockpit repaint. Preserve that quirk rather than deriving a stopwatch.
  if(colon) seconds = (seconds + 1) % (25 * 60);
}
void clock_instrument::update(uint32_t const now) {
  // Missed deadlines produce one update; the original does not catch up.
  if(deadline < now) { refresh(); deadline = now + 30; }
}
std::string clock_instrument::text() const {
  auto const digits{[](unsigned int value){return std::string{static_cast<char>('0' + value / 10), static_cast<char>('0' + value % 10)};}};
  return digits(seconds / 60) + ':' + digits(seconds % 60);
}
bool clock_instrument::colon_visible() const { return colon; }
uint32_t clock_instrument::next_update() const { return deadline; }
void clock_instrument::draw(framebuffer &target, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto box{bounds}; box.left += offset_x; box.right += offset_x; box.top += offset_y; box.bottom += offset_y;
  auto const label{text()};
  int const y{box.top + ((box.bottom - box.top + static_cast<int>(face.height)) >> 1)};
  target.fill(box, 63);
  target.text(face, label.substr(0, 2), box.left + 1, y);
  target.text(face, label.substr(3, 2), box.right - (face.text_width(label.substr(3, 2)) - 1), y);
  if(colon) target.text(face, ":", box.left + ((box.right - box.left) >> 1) - ((face.text_width(":") - 1) >> 1), y);
}
} // namespace earthsiege
