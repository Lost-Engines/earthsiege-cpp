#include "radio_queue.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
bool radio_queue::push(radio_descriptor const &descriptor, uint32_t const text, radio_event event, uint32_t const ticks) {
  if(data.count==data.slots.size()) {
    data.allocated[data.order[data.order.back()]]=false;
    --data.count;
  }
  auto const free{std::ranges::find(data.allocated,false)};
  if(free==data.allocated.end()) return false;
  auto const slot{static_cast<uint8_t>(free-data.allocated.begin())};
  *free=true; event.id=descriptor.id;
  data.slots[slot]={event,descriptor.flags,text,descriptor.priority,
    static_cast<uint32_t>(descriptor.minimum)*60,static_cast<uint32_t>(descriptor.maximum)*60,
    ticks+static_cast<uint32_t>(descriptor.earliest)*60,ticks+static_cast<uint32_t>(descriptor.expiry)*60};
  size_t index{};
  for(;index<data.count;++index) if(descriptor.priority<data.slots[data.order[index]].priority) {
    // The original starts at count-1, leaving the new final index untouched.
    for(size_t i{data.count-1};i>index;--i) data.order[i]=data.order[i-1];
    break;
  }
  data.order[index]=slot; ++data.count;
  return true;
}
void radio_queue::erase(size_t const index) {
  if(index>=data.count) throw std::out_of_range{"radio queue index"};
  data.allocated[data.order[index]]=false;
  for(size_t i{index+1};i<data.count;++i) data.order[i-1]=data.order[i];
  --data.count;
}
void radio_queue::expire(uint32_t const ticks) {
  // The active/front entry belongs to the presentation consumer.
  for(size_t i{data.count};i>1;--i) if(data.slots[data.order[i-1]].expiry<ticks) erase(i-1);
}
radio_message *radio_queue::front() {return data.count ? &data.slots[data.order[0]] : nullptr;}
radio_message &radio_queue::slot(size_t const index) {return data.slots.at(index);}
radio_queue_state const &radio_queue::state() const {return data;}
} // namespace earthsiege
