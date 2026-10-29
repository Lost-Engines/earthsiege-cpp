#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "vectorstorm/vector/vector3.h"
#include "resources/binary_reader.h"

namespace earthsiege {

using world_coordinate = int32_t;
using mission_reference = int16_t;
using native_angle = uint16_t;

enum class terrain_kind : int16_t {
  desert,
  temperate,
  ice,
  urban,
};

enum class actor_kind : int16_t {
  herc,
  flyer,
  base,
};

enum class target_kind : int16_t {
  none = -1,
  group,
  herc,
  flyer,
  base,
};

struct mission_header {
  terrain_kind terrain{};
  int16_t zone{};
  int16_t campaign_mode{};                                                   // GO selects 0 for instant action, 1 for campaign
  int16_t objective_kind{};
  int16_t objective_variant{};                                               // retained shell objective selector
  int16_t unlimited_weapons{};
  int16_t invulnerable_player{};
  int16_t difficulty{};
  int16_t world_variant{};
  int16_t palette_variant{};

  bool free_weapons() const { return campaign_mode == 0 && unlimited_weapons == 1; }
  bool player_is_invulnerable() const { return campaign_mode == 0 && invulnerable_player == 1; }
  int32_t detection_range() const { return world_variant == 1 ? 30000 : 60000; }
};

using mission_counters = std::array<int16_t, 100>;
mission_counters read_mission_counters(bytes source);
std::vector<uint8_t> write_mission_counters(mission_counters const &counters);

struct mission_route {
  std::vector<mission_reference> points;
};

struct trigger_area {
  int16_t kind{};
  mission_reference first_point{};
  int16_t second_point_or_radius{};
};

struct mission_trigger {
  int16_t mode{};
  int16_t test{};
  std::array<mission_reference, 8> areas{};
  int16_t counter{-1};
  int16_t counter_operation{-1};
};

using source_field_mask = std::array<int16_t, 20>;                             // original editor/merge flags retained in the handoff

struct herc_spawn {
  source_field_mask field_mask{};
  int16_t chassis{};
  std::array<int16_t, 10> weapons{};
  mission_reference position{};
  mission_reference heading{};
  int16_t counter{};
  int16_t counter_operation{};
  std::array<int16_t, 10> ammunition{};
  int16_t skill{};
  mission_reference trigger_a{};
  mission_reference trigger_b{};
  int16_t initial_condition{};
};

struct flyer_spawn {
  source_field_mask field_mask{};
  mission_reference position{};
  mission_reference heading{};
  int16_t type{};
  int16_t counter{};
  int16_t counter_operation{};
  mission_reference trigger_a{};
  mission_reference trigger_b{};
  int16_t initial_value{};
};

struct base_spawn {
  int16_t type{};
  mission_reference position{};
  mission_reference heading{};
  int16_t counter{};
  int16_t counter_operation{};
  mission_reference trigger_a{};
  mission_reference trigger_b{};
  int16_t initial_value{};
};

struct group_spawn {
  source_field_mask field_mask{};
  actor_kind kind{};
  int16_t formation{};
  int16_t argument{};
  mission_reference position{};
  mission_reference heading{};
  mission_reference route{};
  std::array<mission_reference, 8> members{};
  int16_t mode{};
  target_kind target_type{};
  mission_reference target{};
  mission_reference trigger{};
  std::array<int16_t, 3> extra{};
};

struct mission_script {
  mission_header header;
  std::vector<vec3<world_coordinate>> points;
  std::vector<int16_t> headings;
  std::vector<mission_route> routes;
  std::vector<trigger_area> areas;
  std::vector<mission_trigger> triggers;
  std::vector<herc_spawn> hercs;
  std::vector<flyer_spawn> flyers;
  std::vector<base_spawn> bases;
  std::vector<group_spawn> groups;
  size_t consumed{};
};

auto read_mission_script(bytes source)->mission_script;
std::vector<uint8_t> write_mission_script(mission_script const &script);
auto decode_mission_heading(int16_t stored)->native_angle;

} // namespace earthsiege
