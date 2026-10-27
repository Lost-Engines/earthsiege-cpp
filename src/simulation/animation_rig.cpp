#include "animation_rig.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
namespace {

animation_list const &require_animation(shape_part const &shape) {
  if(!shape.animation) throw std::invalid_argument{"animation rig requires an articulated shape"};
  return *shape.animation;
}

} // namespace

animation_rig::animation_rig(shape_part const &shape) : source{require_animation(shape)}, model{shape} {
}

size_t animation_rig::add_channel(animation_index const sequence) {
  auto const index{channels.size()};
  channels.emplace_back(source);
  channels.back().reset(sequence);
  priority_order.push_back(index);
  return index;
}

animation_channel &animation_rig::channel(size_t const index) {
  return channels.at(index);
}

animation_channel const &animation_rig::channel(size_t const index) const {
  return channels.at(index);
}

model_instance &animation_rig::instance() {
  return model;
}

model_instance const &animation_rig::instance() const {
  return model;
}

void animation_rig::update_pose() {
  /// Stable order is retained between updates, with lower priorities applied last
  std::stable_sort(priority_order.begin(), priority_order.end(), [&](size_t const a, size_t const b){
    auto const &first{source.sequences.at(static_cast<size_t>(channels[a].state().sequence))};
    auto const &second{source.sequences.at(static_cast<size_t>(channels[b].state().sequence))};
    return first.priority < second.priority;
  });
  for(auto iterator{priority_order.rbegin()}; iterator != priority_order.rend(); ++iterator) channels[*iterator].apply(model);
  model.update();
}

std::span<size_t const> animation_rig::order() const {
  return priority_order;
}

} // namespace earthsiege
