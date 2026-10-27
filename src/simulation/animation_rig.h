#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include "simulation/animation_channel.h"
#include "simulation/model_instance.h"

namespace earthsiege {

/// Own an articulated instance and its independently controlled animation channels
class animation_rig {
  animation_list const &source;
  model_instance model;
  std::vector<animation_channel> channels;
  std::vector<size_t> priority_order;

public:
  explicit animation_rig(shape_part const &shape);
  size_t add_channel(animation_index sequence);
  animation_channel &channel(size_t index);
  animation_channel const &channel(size_t index) const;
  model_instance &instance();
  model_instance const &instance() const;
  void update_pose();
  std::span<size_t const> order() const;
};

} // namespace earthsiege
