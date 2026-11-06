#include "campaign.h"
#include <array>
#include <stdexcept>
namespace earthsiege {
mission_choices read_mission_choices(binary_reader &file) {
  auto const count{file.integer<uint8_t>()};mission_choices result;
  if(!count){if(file.integer<uint8_t>())file.fail("empty filename list terminator");return result;}
  for(unsigned i{};i<count;++i)result.push_back(file.cstring());
  return result;
}
std::vector<mission_choices> read_single_missions(bytes source) {
  binary_reader file{source,"single mission list"};std::vector<mission_choices> out;auto const count{file.integer<uint8_t>()};
  for(unsigned i{};i<count;++i)out.push_back(read_mission_choices(file));
  file.require_end();return out;
}
campaign_table read_campaign_table(bytes source,bool expansion_campaign) {
  binary_reader file{source,"campaign table"};campaign_table out;out.expansion_campaign=expansion_campaign;auto const count{file.integer<uint8_t>()};
  if(count>8)file.fail("campaign graph exceeds original node table");
  for(unsigned i{};i<count;++i){campaign_node node;auto const steps{file.integer<uint8_t>()};for(unsigned j{};j<steps;++j){auto choices{read_mission_choices(file)};if(choices.empty()||choices.size()>2)file.fail("invalid campaign branch count");node.push_back(std::move(choices));}out.nodes.push_back(std::move(node));}
  file.require_end();return out;
}
std::string const &campaign_table::filename(int16_t node,int16_t step,bool branch) const {
  auto const &choices{nodes.at(static_cast<size_t>(node)).at(static_cast<size_t>(step))};return choices.at(choices.size()==1?0:static_cast<size_t>(branch));
}
bool campaign_table::advance(int16_t &node,int16_t &step,bool branch) const {
  constexpr std::array<std::array<int16_t,2>,8> targets{{{1,2},{3,4},{3,-1},{5,6},{5,-1},{7,-1},{7,-1},{99,-1}}};
  auto const &steps{nodes.at(static_cast<size_t>(node))};
  if(step<0||static_cast<size_t>(step)>=steps.size())throw std::out_of_range{"campaign step"};
  ++step;if(static_cast<size_t>(step)<steps.size())return false;
  step=0;node=expansion_campaign?(branch?-1:node==2?99:static_cast<int16_t>(node+1)):targets.at(static_cast<size_t>(node))[branch];return true;
}
bool campaign_failure_branch(int16_t kind,int16_t grade) {
  constexpr std::array<int16_t,10> thresholds{4,4,4,3,3,3,4,3,3,3};return grade>thresholds.at(static_cast<size_t>(kind));
}
} // namespace earthsiege
