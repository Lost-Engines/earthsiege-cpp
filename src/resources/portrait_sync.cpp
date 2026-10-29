#include "portrait_sync.h"
#include <utility>

namespace earthsiege {
std::vector<portrait_frame> read_portrait_sync(bytes source){
  binary_reader file{source,"simulation portrait sync"};
  auto const size{file.integer<uint32_t>()};
  if(size!=file.remaining() || size%2)file.fail("portrait byte count");
  std::vector<portrait_frame> frames;
  while(file.remaining())frames.push_back({file.integer<uint8_t>(),file.integer<uint8_t>()});
  return frames;
}
portrait_animation::portrait_animation(std::vector<portrait_frame> timeline):frames{std::move(timeline)}{}
void portrait_animation::next(){
  if(cursor==frames.size() || frames[cursor].frame==255){current=-1;return;}
  auto const f{frames[cursor++]};current=f.frame;
  deadline=f.duration==255 ? UINT32_MAX : clock+f.duration;
}
void portrait_animation::start(uint32_t const ticks){clock=ticks;cursor=0;next();}
void portrait_animation::advance(uint32_t const ticks){
  auto remaining{ticks-clock};
  while(current>=0 && deadline-clock>0 && deadline-clock<=remaining){
    remaining-=deadline-clock;clock=deadline;next();
  }
  clock+=remaining;
}
int16_t portrait_animation::frame()const{return current;}
uint32_t portrait_animation::next_tick()const{return deadline;}
} // namespace earthsiege
