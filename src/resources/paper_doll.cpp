#include "paper_doll.h"

namespace earthsiege {
namespace {
rectangle bounds(binary_reader &file) {
  return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}
uint32_t count(binary_reader &file, size_t const minimum_size) {
  auto const result{file.integer<uint32_t>()};
  if(result > file.remaining() / minimum_size) file.fail("paper doll count exceeds resource");
  return result;
}
}
paper_doll read_paper_doll(bytes const source) {
  binary_reader file{source, "paper doll"};
  paper_doll result;
  auto const views{count(file, 20)};
  for(uint32_t i{}; i < views; ++i) {
    paper_doll_view view{bounds(file), {}};
    auto const regions{count(file, 28)};
    for(uint32_t j{}; j < regions; ++j) {
      paper_doll_region region{file.integer<int32_t>(), bounds(file), file.integer<int32_t>(), {}};
      auto const mode{file.integer<int32_t>()};
      if(mode < 0 || mode > 3) file.fail("unknown paper doll colour mode");
      region.colour_mode = static_cast<damage_colour_mode>(mode);
      view.regions.push_back(region);
    }
    result.views.push_back(std::move(view));
  }
  auto const mounts{count(file, 20)};
  for(uint32_t i{}; i < mounts; ++i) {
    auto &mount{result.mounts.emplace_back()};
    for(auto &value : mount) value = file.integer<int32_t>();
  }
  file.require_end();
  return result;
}
std::vector<std::array<int32_t, 2>> read_weapon_icon_sizes(bytes const source) {
  binary_reader file{source, "weapon icon sizes"};
  std::vector<std::array<int32_t, 2>> result;
  auto const amount{count(file, 8)};
  for(uint32_t i{}; i < amount; ++i) result.push_back({file.integer<int32_t>(), file.integer<int32_t>()});
  file.require_end();
  return result;
}
} // namespace earthsiege
