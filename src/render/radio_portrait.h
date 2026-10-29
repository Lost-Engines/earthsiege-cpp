#pragma once
#include "resources/artwork.h"
#include "resources/resource_store.h"
#include "render/framebuffer.h"
#include "simulation/random_state.h"

namespace earthsiege {
class radio_portrait {
  std::vector<bitmap> images;
  std::array<std::array<int32_t,2>,28> offsets{};
  std::vector<uint8_t> unused_offset_data;
  bool reduced;
  size_t mouth{},backdrop{};
public:
  radio_portrait(resource_store const &resources,size_t pilot,bool reduced_detail);
  /// Select the original backing frame once per UI update, independently of
  /// window repainting. Reduced detail consumes no UI random numbers.
  void update(size_t mouth_frame,random_state &random);
  void draw(framebuffer &frame,rectangle const &bounds)const;
};
} // namespace earthsiege
