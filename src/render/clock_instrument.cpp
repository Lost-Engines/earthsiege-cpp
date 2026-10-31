#include "clock_instrument.h"
#include <algorithm>

namespace earthsiege {
clock_instrument::clock_instrument(resource_store const &resources, cockpit_layout const &layout, uint32_t const now, bool const initial_colon)
  : bounds{layout.bounds(0x654)}, face{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN"))}, deadline{now + 30}, colon{initial_colon} {
  for(auto &page:retained){
    page.width=static_cast<unsigned>(bounds.right-bounds.left+1);
    page.height=static_cast<unsigned>(bounds.bottom-bounds.top+1);
    page.pixels.assign(page.width*page.height,63);
  }
}

bool clock_instrument::visible_at(int x,int y,bool hidden)const{
  // Original 2b214 tests the two opposite corners, with inclusive y+200.
  return !hidden && ((bounds.left>=x && bounds.left<=x+319 && bounds.top>=y && bounds.top<=y+200) ||
                     (bounds.right>=x && bounds.right<=x+319 && bounds.bottom>=y && bounds.bottom<=y+200));
}
void clock_instrument::paint_retained(unsigned page){
  framebuffer image;draw(image,-bounds.left,-bounds.top);
  auto &out{retained.at(page)};
  for(unsigned y{};y<out.height;++y)for(unsigned x{};x<out.width;++x)
    out.pixels[y*out.width+x]=image.pixels()[y*screen_width+x];
}
void clock_instrument::copy_retained(unsigned source,unsigned target,rectangle region){
  auto const &from{retained.at(source)};auto &to{retained.at(target)};
  for(int y{std::max(bounds.top,region.top)};y<=std::min(bounds.bottom,region.bottom);++y)
    for(int x{std::max(bounds.left,region.left)};x<=std::min(bounds.right,region.right);++x){
      auto const index{static_cast<size_t>((y-bounds.top)*from.width+x-bounds.left)};
      to.pixels[index]=from.pixels[index];
    }
}
void clock_instrument::initialize_pages(){paint_retained(0);paint_retained(1);copy_pending=false;}
void clock_instrument::refresh_retained(unsigned page,bool visible){
  refresh();if(visible)paint_retained(page);
}
void clock_instrument::update_retained(uint32_t now,unsigned source,unsigned target,bool visible,bool preparing){
  // 435dc copies the previously painted page before polling the timer.
  // During the launch shake the pages do not swap: this can copy an older
  // display over a newer clock, even though its logical time has advanced.
  if(copy_pending){if(visible&&!preparing)copy_retained(source,target,bounds);copy_pending=false;}
  if(deadline<now){refresh_retained(target,visible);deadline=now+30;copy_pending=true;}
}
void clock_instrument::draw_retained(framebuffer &target,unsigned page,int x,int y)const{
  scoped_framebuffer_clip clip{target,nullptr};
  target.image(retained.at(page),bounds.left+x,bounds.top+y);
}
void clock_instrument::refresh() {
  colon = !colon;
  // Original 0x43658 wraps the two minute digits at 25, including on a full
  // cockpit repaint. Preserve that quirk rather than deriving a stopwatch.
  if(colon) seconds = (seconds + 1) % (25 * 60);
}
void clock_instrument::update(uint32_t const now) {
  // Missed deadlines produce one update; the original does not catch up.
  if(deadline < now) { refresh(); deadline = now + 30; }
}
std::string clock_instrument::text() const {
  auto const digits{[](unsigned int value){return std::string{static_cast<char>('0' + value / 10), static_cast<char>('0' + value % 10)};}};
  return digits(seconds / 60) + ':' + digits(seconds % 60);
}
bool clock_instrument::colon_visible() const { return colon; }
uint32_t clock_instrument::next_update() const { return deadline; }
void clock_instrument::draw(framebuffer &target, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto box{bounds}; box.left += offset_x; box.right += offset_x; box.top += offset_y; box.bottom += offset_y;
  auto const label{text()};
  int const y{box.top + ((box.bottom - box.top + static_cast<int>(face.height)) >> 1)};
  target.fill(box, 63);
  target.text(face, label.substr(0, 2), box.left + 1, y);
  target.text(face, label.substr(3, 2), box.right - (face.text_width(label.substr(3, 2)) - 1), y);
  if(colon) target.text(face, ":", box.left + ((box.right - box.left) >> 1) - ((face.text_width(":") - 1) >> 1), y);
}
} // namespace earthsiege
