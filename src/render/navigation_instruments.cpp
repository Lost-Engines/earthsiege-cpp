#include "navigation_instruments.h"
#include <algorithm>
#include "simulation/fixed_math.h"
#include "resources/interface_strings.h"

namespace earthsiege {
namespace {
rectangle relative(rectangle bounds, rectangle const &parent) {
  bounds.left += parent.left; bounds.right += parent.left;
  bounds.top += parent.top; bounds.bottom += parent.top;
  return bounds;
}
void cropped_image(framebuffer &target, bitmap const &image, int const x, int const y, int const left, int const right) {
  int const first{std::max(0, left - x)};
  int const last{std::min(static_cast<int>(image.width) - 1, right - x)};
  for(unsigned int row{}; row < image.height; ++row) for(int column{first}; column <= last; ++column) {
    auto const colour{image.pixels[row * image.width + static_cast<unsigned int>(column)]};
    if(colour) target.pixel(x + column, y + static_cast<int>(row), colour);
  }
}
int indicator_position(int16_t const value, int const minimum, int const maximum, int const length) {
  // The original subtracts unsigned shorts, then performs an unsigned division.
  int const fraction{static_cast<uint16_t>(value - minimum)};
  return fraction * length / (maximum - minimum) + (value < 0 ? 1 : 0);
}
} // namespace

navigation_instruments::navigation_instruments(resource_store const &resources, cockpit_layout const &layout)
  : heading_bounds{relative(layout.bounds(0x460), layout.bounds(0x450))},
    torso_bounds{heading_bounds},
    pitch_bounds{relative(layout.bounds(0x470), layout.bounds(0x450))},
    cross{layout.words[0x480 / 4], layout.words[0x484 / 4]},
    heading_art{read_bitmaps(resources.read("simvol0", "dba/HUDHTICK.DBA"))},
    torso_art{read_bitmaps(resources.read("simvol0", "dba/HUDHSLID.DBA"))},
    pitch_art{read_bitmaps(resources.read("simvol0", "dba/HUDVSLID.DBA"))},
    tracking_font{read_font(resources.read("simvol0", "dfn/CPBLACK.DFN"))},
    tracking_label{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR")).at(33).at(0).text},
    heading_first{heading_art.size() - 1}, heading_x{heading_bounds.left} {
  torso_bounds.left += 15; torso_bounds.right -= 15;
  torso_bounds.top += 10; torso_bounds.bottom += 10;
}

void navigation_instruments::update(int16_t const heading, int16_t const torso, int16_t const pitch, bool const disabled) {
  if(!disabled && heading != previous_heading) {
    int const width{heading_bounds.right - heading_bounds.left};
    int const scale{static_cast<int>(heading_art.size()) * width * 1024 / 65536};
    int const angle{static_cast<uint16_t>(-heading)};
    int const progress{(angle * scale) >> 10};
    int tile{progress / width};
    // 0x33d08 retains the separate short-angle compensation term; combining
    // these divisions loses the original tape's integer rounding.
    int distance{progress % width + (((angle * 256 / 65536) * ((scale * 0xe39) >> 10)) >> 8)};
    if(distance >= width) { distance -= width; if(++tile == static_cast<int>(heading_art.size())) tile = 0; }
    heading_first = static_cast<size_t>(tile);
    heading_second = (heading_first + 1) % heading_art.size();
    heading_x = heading_bounds.left - distance;
    previous_heading = heading;
  }
  if(!disabled && torso != previous_torso) {
    auto const delta{wrap_word(previous_torso - torso)};
    torso_indicator = std::clamp(wrap_word(torso_indicator - delta), int16_t{-14563}, int16_t{14563});
    previous_torso = torso;
  }
  if(pitch != previous_pitch) {
    auto const delta{wrap_word(previous_pitch - pitch)};
    pitch_indicator = std::clamp(wrap_word(pitch_indicator - delta), int16_t{-8000}, int16_t{8000});
    previous_pitch = pitch;
  }
}
uint16_t navigation_instruments::display_heading() const { return static_cast<uint16_t>(previous_heading); }
std::array<int16_t, 2> navigation_instruments::indicator_angles() const { return {torso_indicator, pitch_indicator}; }

void navigation_instruments::draw(framebuffer &target, int const offset_x, int const offset_y, bool const tracking) const {
  scoped_framebuffer_clip clip{target, nullptr};
  int const left{heading_bounds.left + offset_x}, right{heading_bounds.right + offset_x};
  cropped_image(target, heading_art.at(heading_first), heading_x + offset_x, heading_bounds.top + offset_y, left, right);
  cropped_image(target, heading_art.at(heading_second), heading_x + offset_x + heading_bounds.right - heading_bounds.left, heading_bounds.top + offset_y, left, right);
  target.image(torso_art.at(0), torso_bounds.left + offset_x, torso_bounds.top - 1 + offset_y, true);
  int const tx{torso_bounds.left + indicator_position(torso_indicator, -14563, 14563, torso_bounds.right - torso_bounds.left)};
  target.image(torso_art.at(torso_indicator > -300 && torso_indicator < 300 ? 2 : 1), tx - 15 + offset_x, torso_bounds.top + offset_y, true);
  target.image(pitch_art.at(0), pitch_bounds.left + offset_x, pitch_bounds.top + offset_y, true);
  int const py{pitch_bounds.top + indicator_position(pitch_indicator, -8000, 8000, pitch_bounds.bottom - pitch_bounds.top)};
  target.image(pitch_art.at(pitch_indicator > -400 && pitch_indicator < 400 ? 2 : 1), pitch_bounds.left + offset_x, py - 7 + offset_y, true);
  int const x{cross[0] + offset_x}, y{cross[1] + offset_y};
  target.line(x + 3, y, x + 6, y, 56); target.line(x - 6, y, x - 3, y, 56);
  target.line(x, y - 6, x, y - 3, 56); target.line(x, y + 3, x, y + 6, 56);
  if(tracking) draw_tracking(target, offset_x, offset_y);
}

void navigation_instruments::draw_tracking(framebuffer &target, int const offset_x, int const offset_y) const {
    scoped_framebuffer_clip clip{target, nullptr};
    rectangle const label{heading_bounds.left - 3 + offset_x, heading_bounds.top + 10 + offset_y,
                          heading_bounds.left + 12 + offset_x, heading_bounds.top + 15 + offset_y};
    target.fill(label, 56);
    target.text(tracking_font, tracking_label, label.left + ((label.right - label.left) >> 1) - ((tracking_font.text_width(tracking_label) - 1) >> 1),
                label.top + ((label.bottom - label.top + static_cast<int>(tracking_font.height)) >> 1));
}

} // namespace earthsiege
