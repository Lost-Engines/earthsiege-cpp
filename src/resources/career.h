#pragma once
#include "resources/player_roster.h"
#include <array>
#include <optional>

namespace earthsiege {

/// Inventory values are in linked-list traversal order, newest at the front.
/// The original reader pushes to the head, so reading reverses the disk order.
using armory=std::array<std::vector<int16_t>,17>;
armory read_armory(binary_reader &file);
std::vector<uint8_t> write_armory(armory const &inventory);

struct hangar {
  int16_t capacity{8};
  int16_t selected{-1};
  int16_t construction_slot{-1};
  int16_t construction_remaining{};
  int32_t construction_value{};
  std::vector<player_herc> hercs;
  player_herc *find(int16_t slot);
  player_herc const *find(int16_t slot) const;
};
hangar read_hangar(binary_reader &file);
hangar read_hangar(bytes source);
std::vector<uint8_t> write_hangar(hangar const &roster);

struct crew_member {
  int16_t name{},variant{},next_name{},rotation{},hangar_slot{-1};
  uint8_t active{};
  int16_t skill{},rank{},health{100};
  std::array<int16_t,3> current{},total{};
  int16_t prior_skill{},experience{};
  int32_t budget_share{};
};
struct technology_entry { int16_t early{-1},late{-1};uint8_t available{}; };
struct technology_state {
  int16_t last_type{},last_index{};
  std::array<technology_entry,25> entries{};
};
struct career_state {
  uint8_t valid{},current{};
  int16_t node_progress{},screen{},node{},step{},missions{},mission_kind{};
  int16_t selected_crew{},crew_count{},crew_cursor{};
  std::array<crew_member,4> crew{};
  technology_state technology;
  int32_t budget{};
  std::array<int16_t,100> counters{};
  int16_t repair_policy{};
};
struct career_record {
  uint8_t active{};
  int16_t mode{};
  std::array<int16_t,12> names{};
  std::array<career_state,41> states{};
  std::array<armory,41> inventories{};
};
career_state read_career_state(binary_reader &file);
std::vector<uint8_t> write_career_state(career_state const &state);
career_record read_career(bytes source);
std::vector<uint8_t> write_career(career_record const &career);
/// GO 0x168b0's working-state update and history copy, before serialization.
void checkpoint_career(career_record &career,size_t history,int16_t node_progress,
                       int32_t budget,std::array<int16_t,100> const &counters,int16_t repair_policy);

} // namespace earthsiege
