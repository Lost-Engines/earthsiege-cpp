#pragma once
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"

namespace earthsiege {
/// Original external-camera viewport (afb3c/afb4c) and status painter (58b68).
class external_view_artwork {
  font face;
  std::vector<interface_string> labels;
  std::string player_label;
public:
  explicit external_view_artwork(resource_store const &);
  static rectangle constexpr bounds{0,30,319,170};
  static vec2<int32_t> constexpr centre{160,105};
  static screen_mask clipping(rectangle);
  void draw(framebuffer &,bool camera_control,std::string_view target={})const;
};
}
