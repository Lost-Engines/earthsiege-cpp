#pragma once
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"
#include "render/framebuffer.h"
#include <span>

namespace earthsiege {
/// The original idle damage/order labels, pilot name and five static frames.
class wingmate_portrait {
  font red,blue;
  interface_strings strings;
  std::vector<interface_string> names;
  std::vector<bitmap> noise;
  void label(framebuffer &target,rectangle bounds,font const &face,std::string_view text,uint8_t background=0)const;
public:
  explicit wingmate_portrait(resource_store const &resources);
  std::string const &name(size_t pilot)const;
  void draw_name(framebuffer &target,rectangle portrait,size_t pilot,size_t slot)const;
  void draw_idle(framebuffer &target,rectangle portrait,size_t pilot,size_t slot,std::span<int16_t const,19> armour,int16_t orders)const;
  void draw_static(framebuffer &target,rectangle portrait,size_t pilot,size_t slot,size_t frame)const;
};
} // namespace earthsiege
