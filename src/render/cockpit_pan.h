#pragma once
#include "resources/cockpit.h"
#include "resources/model.h"
#include <span>

namespace earthsiege {
/// Original 22904 scanout positions, relative to the current drawing page.
std::vector<vec2<int32_t>> cockpit_pan_path(int action,int view,
    std::array<cockpit_window,4> const &windows);

/// The original four 320x200 VGA pages share a 640x400 surface. The lower
/// cockpit begins at its authored VUE offset, which need not be exactly 200.
class cockpit_surface {
  std::array<uint8_t,640*400> pixels_;
  std::array<bool,2> retained_edge{};
public:
  cockpit_surface(){pixels_.fill(63);}
  void restore(std::span<uint8_t const> source);
  void store(framebuffer const &frame,int x,int y);
  void sample(framebuffer &frame,int x,int y)const;
  std::span<uint8_t const> pixels()const {return pixels_;}
  void copy_page(int source_x,int target_x,rectangle bounds);
  void restore_from_external(int source_x,int target_x);
  void prepare_artwork(int x){retained_edge.at(static_cast<size_t>(x/320))=false;}
  void restore_artwork_edge(framebuffer &frame,int x,int y)const;
};
} // namespace earthsiege
