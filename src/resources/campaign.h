#pragma once
#include "resources/binary_reader.h"
#include "resources/edition.h"
#include <string>
#include <vector>

namespace earthsiege {
using mission_choices=std::vector<std::string>;
using campaign_node=std::vector<mission_choices>;
struct campaign_table {
  std::vector<campaign_node> nodes;
  bool expansion_campaign{};
  std::string const &filename(int16_t node,int16_t step,bool branch) const;
  /// GO 0x141d0: true on changing graph nodes, including terminal nodes -1/99.
  bool advance(int16_t &node,int16_t &step,bool branch) const;
};
campaign_table read_campaign_table(bytes source,bool expansion_campaign=false);
mission_choices read_mission_choices(binary_reader &file);
std::vector<mission_choices> read_single_missions(bytes source);
bool campaign_failure_branch(int16_t mission_kind,int16_t result_grade);
} // namespace earthsiege
