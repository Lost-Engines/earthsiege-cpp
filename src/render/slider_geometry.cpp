#include "slider_geometry.h"
#include <algorithm>
#include "simulation/fixed_math.h"

namespace earthsiege {

int slider_geometry::scale() const {
  int const travel{(vertical ? bounds.bottom - bounds.top : bounds.right - bounds.left) - thumb_extent};
  return travel * 65536 / (maximum - minimum);
}
int slider_geometry::boundary_for_value(int const value) const {
  int const extent{wrap_long(static_cast<int64_t>(std::clamp(value, minimum, maximum) - minimum) * scale()) >> 16};
  return vertical ? bounds.bottom - extent : bounds.left + extent;
}
int slider_geometry::clamp_boundary(int const x, int const y) const {
  return vertical ? std::clamp(y, bounds.top + thumb_extent, bounds.bottom) : std::clamp(x, bounds.left, bounds.right - thumb_extent);
}
rectangle slider_geometry::thumb_at(int const boundary) const {
  auto result{bounds};
  if(vertical) { result.bottom = boundary; result.top = boundary - thumb_extent; }
  else { result.left = boundary; result.right = boundary + thumb_extent; }
  return result;
}
int slider_geometry::axis_value(rectangle const &painted_thumb) const {
  int const distance{vertical ? painted_thumb.top - bounds.top : painted_thumb.left - bounds.left};
  return wrap_word(distance * 65536 / scale() + minimum);
}

} // namespace earthsiege
