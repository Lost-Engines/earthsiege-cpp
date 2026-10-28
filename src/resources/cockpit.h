#pragma once
#include <array>
#include <string>
#include <vector>
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "resources/resource_store.h"

namespace earthsiege {

enum class cockpit_view : unsigned int { forward, lower, right, left };
struct cockpit_window {
  rectangle bounds;
  std::array<int32_t, 2> projection_origin;
  std::array<int32_t, 2> instrument_origin;
  std::array<int32_t, 2> projection_centre() const;
  screen_mask mask;
  std::vector<uint8_t> unused_mask_bytes;
};

/// GAU retains the complete original widget records, with only parent rectangles rebased.
struct cockpit_layout {
  std::array<int32_t, 425> words;
  unsigned int weapon_count() const;
  rectangle bounds(size_t byte_offset) const;
};

struct cockpit_artwork {
  std::array<bitmap, 3> panels;
  std::array<cockpit_window, 4> windows;
  cockpit_layout layout;
  void draw(framebuffer &target, cockpit_view view) const;
};

screen_mask read_screen_mask(bytes source, std::vector<uint8_t> *unused = nullptr);
cockpit_layout read_cockpit_layout(bytes source);
std::array<cockpit_window, 4> read_cockpit_windows(bytes source);
cockpit_artwork read_cockpit(resource_store const &resources, std::string const &name);
void apply_cockpit_colours(palette &world, palette const &cockpit, unsigned int chassis);

} // namespace earthsiege
