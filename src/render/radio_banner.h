#pragma once
#include "resources/artwork.h"
#include "resources/resource_store.h"
#include "render/framebuffer.h"
#include <optional>

namespace earthsiege {
class radio_banner {
  font face;
public:
  explicit radio_banner(resource_store const &resources);
  std::string format(std::string_view name, std::string_view message) const;
  void draw_system(framebuffer &frame,std::string_view text,rectangle bounds,bool show=true)const;
  rectangle bounds(std::string_view formatted) const;
  void draw(framebuffer &frame, std::string_view formatted, std::optional<size_t> speaker) const;
};
} // namespace earthsiege
