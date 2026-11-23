#include "palette_device.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
void palette_device::changed(unsigned first, unsigned count) {
  if(!count || first >= 256 || count > 256 - first) throw std::invalid_argument{"palette update bounds"};
  unsigned last{first + count - 1};
  if(dirty) {
    dirty->first = std::min(dirty->first, first);
    dirty->second = std::max(dirty->second, last);
  } else {
    dirty = {{first, last}};
  }
}
void palette_device::upload(palette const &colours, unsigned first, unsigned count) {
  // The game writes one red component before selecting the next range.
  // DOSBox updates the screen palette only after a complete RGB triplet.
  registers[next][0] = 0;
  for(unsigned i{first}; i < first + count; ++i) {
    registers[i] = colours.dac_colours[i];
    displayed.dac_colours[i] = registers[i];
    displayed.colours[i] = display_colour(displayed.dac_colours[i]);
  }
  next = (first + count) & 255;
}
void palette_device::present(palette const &colours) {
  displayed.shade_ramps = colours.shade_ramps;
  if(source != &colours) {
    source = &colours;
    upload(colours, 0, 256);
  } else if(dirty) {
    upload(colours, dirty->first, dirty->second - dirty->first + 1);
  } else {
    upload(colours, 0, 1);
  }
  dirty.reset();
}
} // namespace earthsiege
