#include "mission_conditions.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include "simulation/random_state.h"

namespace earthsiege {

auto read_conditions(binary_reader &file)->std::vector<mission_condition> {
  /// Read the source condition table after the mission's revision word.
  auto const count{file.integer<uint16_t>()};
  std::vector<mission_condition> result;
  result.reserve(count);
  for(unsigned int index{}; index < count; ++index) {
    mission_condition entry{
      .id{file.integer<int16_t>()},
      .parent{file.integer<int16_t>()},
      .rule{},
      .scratch{},
      .result{},
    };
    auto const kind{file.integer<int16_t>()};
    auto const first{file.integer<int16_t>()};
    auto const second{file.integer<int16_t>()};
    auto const third{file.integer<int16_t>()};
    entry.scratch = file.integer<int16_t>();
    switch(kind) {
      case 0:
        entry.rule = flag_test{
          .flag{first},
          .comparison{static_cast<flag_comparison>(second)},
          .expected{third},
        };
        break;
      case 1:
        entry.rule = random_draw{
          .limit{first},
          .source_result{second},
          .reserved{third},
        };
        break;
      case 2:
        entry.rule = random_interval{
          .first{std::bit_cast<uint16_t>(first)},
          .last{std::bit_cast<uint16_t>(second)},
          .reserved{third},
        };
        break;
      case 3:
        entry.rule = random_group{
          .group{first},
          .reserved{second},
          .limit{third},
        };
        break;
      default: file.fail("unknown mission condition kind");
    }
    result.push_back(std::move(entry));
  }
  return result;
}

auto condition_enabled(std::span<mission_condition const> active, int16_t id)->bool {
  return id == -1 || std::ranges::any_of(active, [id](auto const &condition){ return condition.id == id; });
}

auto evaluate_conditions(std::span<mission_condition const> source, std::span<int16_t const> flags, random_state &random)->std::vector<mission_condition> {
  /// GO 0x3276c retains eligible conditions in source order; groups bind later.
  std::vector<mission_condition> active;
  for(auto const &entry : source) {
    auto candidate{entry};
    bool accept{};
    bool const parent{condition_enabled(active, entry.parent)};
    if(auto const *rule{std::get_if<flag_test>(&entry.rule)}; rule && parent) {
      if(rule->flag < 0 || static_cast<unsigned int>(rule->flag) >= flags.size()) throw std::runtime_error{"mission condition refers to an invalid campaign flag"};
      auto const value{flags[static_cast<unsigned int>(rule->flag)]};
      switch(rule->comparison) {
        case flag_comparison::equal: accept = value == rule->expected; break;
        case flag_comparison::not_equal: accept = value != rule->expected; break;
        case flag_comparison::less: accept = value < rule->expected; break;
        case flag_comparison::greater: accept = value > rule->expected; break;
      }
      candidate.result = static_cast<int16_t>(accept);
    } else if(auto const *draw{std::get_if<random_draw>(&entry.rule)}; draw && parent) {
      candidate.result = random.bounded(draw->limit);
      accept = true;
    } else if(auto const *interval{std::get_if<random_interval>(&entry.rule)}) {
      for(auto const &previous : active) {
        if(previous.id != entry.parent) continue;
        if(std::holds_alternative<random_group>(previous.rule)) accept = true;
        else if(std::holds_alternative<random_draw>(previous.rule) && previous.result) {
          accept = interval->first <= *previous.result && *previous.result <= interval->last;
        }
        break;
      }
    } else if(std::holds_alternative<random_group>(entry.rule)) accept = parent;
    if(accept) active.push_back(std::move(candidate));
  }
  return active;
}

} // namespace earthsiege
