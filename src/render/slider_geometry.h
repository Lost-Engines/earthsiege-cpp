#pragma once
#include "render/framebuffer.h"

namespace earthsiege {

/// Geometry and fixed-point rounding shared by the original slider painters and input.
struct slider_geometry {
  rectangle bounds;
  bool vertical;
  int thumb_extent;
  int minimum;
  int maximum;
  int scale() const;
  int boundary_for_value(int value) const;
  int clamp_boundary(int x, int y) const;
  rectangle thumb_at(int boundary) const;
  /// Native release reads the last painted rectangle, increasing down/right.
  /// Vertical throttle callbacks negate this value; setters measure from bottom.
  int axis_value(rectangle const &painted_thumb) const;
};

} // namespace earthsiege
