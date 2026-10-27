#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include "resources/mission_script.h"

namespace earthsiege {

struct spatial_area {
  bool rectangular{false};
  vec3<int32_t> first;
  vec3<int32_t> second;
  int32_t radius{0};
  bool contains(vec3<int32_t> const &position) const;
};

enum class trigger_audience : int16_t {
  player,
  player_group,
  side_zero,
  side_one,
  hercs,
  flyers,
};

struct trigger_group {
  actor_kind kind{actor_kind::herc};
  uint8_t side{0};
  bool waiting{false};
  std::span<vec3<int32_t> const> positions;
};

/// Own permanent trigger latches; mission counters remain owned by the mission
class mission_triggers {
  struct trigger {
    trigger_audience audience{trigger_audience::player};
    std::vector<size_t> areas;
    int16_t counter{-1};
    int16_t operation{-1};
    bool latched{false};
  };
  std::vector<spatial_area> areas;
  std::vector<trigger> triggers;

public:
  explicit mission_triggers(mission_script const &script);
  bool latched(size_t index) const;
  void activate(size_t index, std::span<int16_t> counters);
  bool enter(size_t index, vec3<int32_t> const &position, std::span<int16_t> counters);
  void scan(vec3<int32_t> const &player, size_t player_group, std::span<trigger_group const> groups, std::span<int16_t> counters);
};

} // namespace earthsiege
