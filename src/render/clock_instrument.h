#pragma once
#include <string>
#include "resources/cockpit.h"

namespace earthsiege {

/// Original cockpit clock: a blinking colon, with one second per lit phase.
class clock_instrument {
  rectangle bounds;
  font face;
  uint32_t deadline;
  unsigned int seconds{};
  bool colon{};
  std::array<bitmap,2> retained;
  bool copy_pending{};
public:
  clock_instrument(resource_store const &resources, cockpit_layout const &layout, uint32_t now = 0, bool initial_colon = true);
  void refresh();
  bool visible_at(int origin_x,int origin_y,bool hidden=false)const;
  void paint_retained(unsigned page);
  void copy_retained(unsigned source,unsigned target,rectangle region);
  void initialize_pages();
  void refresh_retained(unsigned page,bool visible=true);
  void update_retained(uint32_t now,unsigned source,unsigned target,bool visible=true,bool preparing=false);
  void draw_retained(framebuffer &target,unsigned page,int offset_x=0,int offset_y=0)const;
  void update(uint32_t now);
  std::string text() const;
  bool colon_visible() const;
  uint32_t next_update() const;
  void draw(framebuffer &target, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
