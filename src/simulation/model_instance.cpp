#include "model_instance.h"
#include <algorithm>
#include <stdexcept>
#include "model_math.h"

namespace earthsiege {

model_instance::model_instance(shape_part const &shape) :
  source{shape},
  cells(shape.cell_frame_counts.size(), 0) {
  /// Share immutable model data while retaining independent pose and cell state
  reset_pose();
}

void model_instance::reset_pose() {
  /// Restore the original zero-cell-independent skeletal defaults
  if(!source.animation) {
    world = source.transforms;
    return;
  }
  auto const &animation{*source.animation};
  local.clear();
  for(auto const index : animation.defaults) local.emplace_back(animation.poses.at(index));
  world.resize(local.size());
  changed.assign(local.size(), true);
  update();
}

void model_instance::apply_frame(unsigned int const sequence, unsigned int const frame, unsigned int const next_sequence, unsigned int const next_frame, int16_t const fraction) {
  /// Blend only the channel's named parts, leaving other channels' poses intact
  if(!source.animation) throw std::runtime_error{"shape has no articulated animations"};
  auto const &animation{*source.animation};
  auto const &from{animation.sequences.at(sequence)};
  auto const &to{animation.sequences.at(next_sequence)};
  if(frame >= from.frames.size() || next_frame >= to.frames.size() || from.parts.size() != to.parts.size()) throw std::runtime_error{"invalid animation frame pair"};
  for(size_t index{0}; index != from.parts.size(); ++index) {
    auto const node{from.parts[index]};
    if(node == 0) continue;
    auto const &a{animation.poses.at(from.pose_indices.at(frame * from.parts.size() + index))};
    auto const &b{animation.poses.at(to.pose_indices.at(next_frame * to.parts.size() + index))};
    local.at(node) = interpolate_pose(a, b, fraction);
    changed.at(node) = true;
  }
}

void model_instance::update() {
  /// Rebuild dirty descendants in authored hierarchy order
  if(!source.animation) return;
  for(auto const &relation : source.animation->relations) {
    if(relation.node < 0 || static_cast<size_t>(relation.node) >= local.size()) throw std::runtime_error{"invalid animation hierarchy node"};
    auto const node{static_cast<size_t>(relation.node)};
    bool const has_parent{relation.parent >= 0};
    auto const parent{static_cast<size_t>(relation.parent)};
    if(has_parent && parent >= world.size()) throw std::runtime_error{"invalid animation hierarchy parent"};
    if(!changed[node] && !(has_parent && changed[parent])) continue;
    auto const &pose{local[node]};
    auto transform{rotation_transform(pose.rotation)};
    transform.translation = vec3<int32_t>{pose.translation};
    world[node] = has_parent ? compose_transforms(transform, world[parent]) : transform;
    changed[node] = true;
  }
  std::ranges::fill(changed, false);
  if(!world.empty()) world[0] = model_transform{};                               // root motion is accumulated separately
}

std::span<animation_pose const> model_instance::poses() const {
  return local;
}
std::span<model_transform const> model_instance::transforms() const {
  return world;
}
std::span<uint16_t> model_instance::cell_frames() {
  return cells;
}
std::span<uint16_t const> model_instance::cell_frames() const {
  return cells;
}

void model_instance::restore(model_instance const &saved) {
  if(&source != &saved.source) throw std::invalid_argument{"cannot restore a different animation source"};
  local = saved.local;
  world = saved.world;
  changed = saved.changed;
}

} // namespace earthsiege
