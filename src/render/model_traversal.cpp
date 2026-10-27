#include "model_traversal.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

auto wrap(int64_t value)->int32_t { return std::bit_cast<int32_t>(static_cast<uint32_t>(value)); }

class bsp_walker {
  bsp_part const &part;
  vec3<int32_t> const &eye;
  std::span<model_transform const> transforms;
  std::vector<uint16_t> order;

  void child(int16_t reference, unsigned int depth) {
    if(reference < 0) return;
    unsigned int constexpr leaf_bit{0x4000};
    unsigned int constexpr index_mask{leaf_bit - 1};
    if(static_cast<unsigned int>(reference) & leaf_bit) {
      auto const index{static_cast<uint16_t>(static_cast<unsigned int>(reference) & index_mask)};
      if(index >= part.children.size()) throw std::runtime_error{"BSP child index exceeds part count"};
      order.push_back(index);
    } else node(static_cast<unsigned int>(reference), depth);
  }

  void node(unsigned int index, unsigned int depth) {
    if(index >= part.nodes.size() || depth > part.nodes.size()) throw std::runtime_error{"invalid or cyclic model BSP"};
    auto const &plane{part.nodes[index]};
    int32_t distance{};
    if(plane.transform < 0) {
      distance = wrap(static_cast<int64_t>(eye.x) * plane.normal.x + static_cast<int64_t>(eye.y) * plane.normal.y + static_cast<int64_t>(eye.z) * plane.normal.z);
    } else {
      auto const transform{static_cast<unsigned int>(plane.transform)};
      if(transform >= transforms.size()) throw std::runtime_error{"BSP transform index exceeds pose count"};
      distance = inverse_plane_distance(eye, transforms[transform], plane.normal);
    }
    distance = wrap(static_cast<int64_t>(distance) - plane.constant);
    // Far half-space first; the authored BSP, rather than a depth buffer or
    // polygon-centre sort, determines the original painter order.
    child(distance < 0 ? plane.front : plane.back, depth + 1);
    child(distance < 0 ? plane.back : plane.front, depth + 1);
  }

public:
  bsp_walker(bsp_part const &source, vec3<int32_t> const &view, std::span<model_transform const> poses) : part{source}, eye{view}, transforms{poses} {}
  auto run()->std::vector<uint16_t> {
    if(!part.nodes.empty()) node(0, 0);
    return std::move(order);
  }
};

auto approximate_distance(vec3<int32_t> const &point)->int32_t {
  std::array<int64_t, 3> components{point.x, point.y, point.z};
  for(auto &value : components) if(value < 0) value = -value;
  std::ranges::sort(components);
  return wrap(components[2] + (components[0] >> 2) + (components[1] >> 2) + (components[1] >> 4) + (components[1] >> 5));
}

auto projected_radius(int16_t radius, vec3<int32_t> const &eye, model_detail const &detail)->int32_t {
  int32_t const distance{std::max(1, approximate_distance(eye) - radius)};
  int32_t projected{wrap(static_cast<int64_t>(radius) * (int64_t{1} << detail.projection_shift)) / distance};
  if(projected == 0) projected = 1;
  int64_t const scaled{static_cast<int64_t>(projected) * detail.scale};
  // Preserve the signed-product rounding instruction used by 0x5e818.
  uint32_t const rounding{(scaled == static_cast<int32_t>(scaled) ? 0U : ~0U) ^ 0x200U};
  projected = wrap((scaled + rounding) >> 10);
  return projected;
}

} // namespace

auto bsp_draw_order(bsp_part const &part, vec3<int32_t> const &eye, std::span<model_transform const> transforms)->std::vector<uint16_t> {
  return bsp_walker{part, eye, transforms}.run();
}

auto cell_draw_index(cell_part const &part, std::span<uint16_t const> frames)->unsigned int {
  if(part.sequence < 0 || static_cast<unsigned int>(part.sequence) >= frames.size() || part.children.empty()) throw std::runtime_error{"invalid model cell animation"};
  return frames[static_cast<unsigned int>(part.sequence)] % static_cast<unsigned int>(part.children.size());
}

auto actor_detail_index(size_t const model_count, std::span<int32_t const> const thresholds, int16_t const radius, vec3<int32_t> const &eye, model_detail const &detail)->unsigned int {
  if(!model_count || model_count > thresholds.size() + 1 || detail.projection_shift > 30) throw std::runtime_error{"invalid actor detail selection"};
  auto const projected{projected_radius(radius, eye, detail)};
  auto index{static_cast<unsigned int>(std::min(static_cast<size_t>(detail.bias), model_count - 1))};
  size_t threshold{0};
  while(index + 1 < model_count && thresholds[threshold] > projected) {
    ++index;
    ++threshold;
  }
  return index;
}

auto detail_draw_index(detail_part const &part, int16_t radius, vec3<int32_t> const &eye, model_detail const &detail)->unsigned int {
  if(part.children.empty() || part.children.size() != part.distances.size() || detail.bias >= part.children.size() || detail.projection_shift > 30) throw std::runtime_error{"invalid model detail selection"};
  auto const projected{projected_radius(radius, eye, detail)};
  unsigned int index{detail.bias};
  while(index + 1 < part.children.size() && projected > part.distances[index]) ++index;
  return index - detail.bias;
}

} // namespace earthsiege
