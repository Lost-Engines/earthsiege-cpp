#include "scene_order.h"
#include <array>
#include "simulation/fixed_math.h"

namespace earthsiege {
namespace {

vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return vec3<int32_t>{wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y), wrap_long(static_cast<int64_t>(a.z) - b.z)};
}

vec3<int32_t> bounded(vec3<int32_t> value) {
  int constexpr maximum{32000};
  while(fast_distance(value) > maximum) {
    for(unsigned int axis{0}; axis != 3; ++axis) value[axis] >>= 1;
  }
  return value;
}

vec3<int32_t> cross(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return vec3<int32_t>{
    wrap_long(static_cast<int64_t>(a.y) * b.z - static_cast<int64_t>(a.z) * b.y),
    wrap_long(static_cast<int64_t>(a.z) * b.x - static_cast<int64_t>(a.x) * b.z),
    wrap_long(static_cast<int64_t>(a.x) * b.y - static_cast<int64_t>(a.y) * b.x),
  };
}

bool positive_side(vec3<int32_t> const &normal, vec3<int32_t> const &point) {
  return static_cast<int64_t>(normal.x) * point.x + static_cast<int64_t>(normal.y) * point.y + static_cast<int64_t>(normal.z) * point.z >= 0;
}

bool inside(vec3<int32_t> const &point, scene_order_entry const &entry) {
  auto const &segment{*entry.segment};
  return fast_distance(difference(point, entry.position)) < (fast_distance(difference(segment.second, segment.first)) >> 1);
}

struct separating_plane {
  vec3<int32_t> normal;
  bool eye_side{false};
};

struct painter_node {
  std::array<std::optional<size_t>, 2> children;
  std::optional<separating_plane> plane;
};

class painter_tree {
  std::span<scene_order_entry const> entries;
  vec3<int32_t> const &eye;
  std::vector<painter_node> nodes;
  std::optional<size_t> root;

  std::optional<bool> relative_to_segment(scene_order_entry const &point, size_t const owner) {
    auto const &entry{entries[owner]};
    if(point.distance > entry.distance || (!inside(eye, entry) && !inside(point.position, entry))) return std::nullopt;
    auto const &segment{*entry.segment};
    auto &plane{nodes[owner].plane};
    if(!plane) {
      auto const axis{bounded(difference(segment.second, segment.first))};
      auto const view{bounded(difference(eye, segment.first))};
      auto const normal{bounded(cross(axis, bounded(cross(axis, view))))};
      plane = separating_plane{.normal{vec3<int32_t>{normal}}, .eye_side{positive_side(normal, view)}};
    }
    return positive_side(plane->normal, bounded(difference(point.position, segment.first))) == plane->eye_side;
  }

  bool compare(size_t const a, size_t const b) {
    auto const &first{entries[a]};
    auto const &second{entries[b]};
    if(first.segment && second.segment) {
      /// If an endpoint enters the other object's extent, compare that point with its plane
      auto const endpoint{[](scene_order_entry const &owner, scene_order_entry const &other)->vec3<int32_t> const * {
        if(inside(other.segment->first, owner)) return &other.segment->first;
        if(inside(other.segment->second, owner)) return &other.segment->second;
        return nullptr;
      }};
      size_t owner{a}, point_index{b};
      auto position{endpoint(first, second)};
      if(!position) {
        owner = b;
        point_index = a;
        position = endpoint(second, first);
      }
      if(!position) return first.distance < second.distance;
      auto point{entries[point_index]};
      point.position = vec3<int32_t>{*position};
      point.segment.reset();
      bool const result{relative_to_segment(point, owner).value_or(point.distance < entries[owner].distance)};
      return point_index == b ? !result : result;
    }
    if(!first.segment && !second.segment) return first.distance < second.distance;
    auto const owner{second.segment ? b : a};
    auto const &point{second.segment ? first : second};
    return relative_to_segment(point, owner).value_or(first.distance < second.distance);
  }

  void insert(size_t const index) {
    if(!root) {
      root = index;
      return;
    }
    auto current{*root};
    for(;;) {
      auto &child{nodes[current].children[compare(index, current) ? 1 : 0]};
      if(!child) {
        child = index;
        return;
      }
      current = *child;
    }
  }

public:
  painter_tree(std::span<scene_order_entry const> const source, vec3<int32_t> const &observer) : entries{source}, eye{observer}, nodes(source.size()) {
  }

  std::vector<size_t> order() {
    for(size_t index{0}; index != entries.size(); ++index) if(entries[index].segment) insert(index);
    for(size_t index{0}; index != entries.size(); ++index) if(!entries[index].segment) insert(index);
    std::vector<size_t> result;
    result.reserve(entries.size());
    std::vector<size_t> pending;
    auto current{root};
    while(current || !pending.empty()) {
      while(current) {
        pending.push_back(*current);
        current = nodes[*current].children[0];
      }
      auto const index{pending.back()};
      pending.pop_back();
      result.push_back(index);
      current = nodes[index].children[1];
    }
    return result;
  }
};

} // anonymous namespace

std::vector<size_t> scene_painter_order(std::span<scene_order_entry const> const entries, vec3<int32_t> const &eye) {
  return painter_tree{entries, eye}.order();
}

} // namespace earthsiege
