#pragma once
#include "render/framebuffer.h"
#include "resources/artwork.h"
namespace earthsiege {
/// Original DCI bitmap and signed hotspot, drawn over the presented page.
class software_cursor {
  bitmap image;
  int32_t hot_x{},hot_y{};
public:
  explicit software_cursor(bytes data);
  void draw(framebuffer &target,int x,int y)const;
};
}
