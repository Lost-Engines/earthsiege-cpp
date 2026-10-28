#include <cstdlib>
#include <iostream>
#include "shell/mfd_controller.h"
#include "resources/resource_store.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 2) return EXIT_FAILURE;
  auto const source{read_file(argv[1])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMFD001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; mfd_controller controller{1};
  auto const compare{[&](int const step){
    auto const &actual{controller.state()}; bool same{true};
    same &= file.integer<int32_t>() == static_cast<int32_t>(actual.page);
    same &= file.integer<int32_t>() == static_cast<int32_t>(actual.range_index);
    same &= file.integer<int32_t>() == static_cast<int32_t>(actual.selected_actor.value_or(0));
    same &= file.integer<int32_t>() == static_cast<int32_t>(actual.choice);
    same &= file.integer<uint8_t>() == actual.pending_paint;
    for(auto const flag : actual.refresh) same &= file.integer<uint8_t>() == flag;
    same &= file.integer<uint32_t>() == actual.deadline;
    same &= file.integer<int16_t>() == static_cast<int16_t>(actual.previous_page);
    same &= file.integer<int16_t>() == (actual.automatic_camera ? 5 : -1);
    same &= file.integer<uint8_t>() == actual.allow_camera_switch;
    for(auto const enabled : mfd_enabled_buttons(actual.page)) same &= file.integer<int16_t>() == (enabled ? 0 : 2);
    if(!same) file.fail("MFD state differs at step " + std::to_string(step));
  }};
  compare(-1);
  for(unsigned int i{}; i < count; ++i) {
    auto const now{file.integer<uint32_t>()}; auto const mode{file.integer<int32_t>()};
    auto const flags{file.integer<uint8_t>()}; auto const scroll{file.integer<int8_t>()};
    auto const disabled{file.integer<uint8_t>()}; auto const paints{file.integer<uint8_t>()};
    std::vector<mfd_paint> expected;
    for(unsigned int j{}; j < paints; ++j) {
      bool const full{file.integer<uint8_t>() != 0}; auto const page{static_cast<mfd_page>(file.integer<uint8_t>())};
      expected.push_back({page, full});
    }
    std::optional<size_t> const target{flags & 8 ? std::optional<size_t>{2} : std::nullopt};
    std::array<communications_choice, 6> choices;
    for(size_t j{}; j < choices.size(); ++j) choices[j].disabled = disabled & (1 << j);
    if(mode >= 0) controller.select(static_cast<mfd_page>(mode), 1, target);
    if(flags & 16) controller.button(8, 1, target, choices);
    if(scroll) controller.button(scroll > 0 ? 12 : 11, 1, target, choices);
    auto const actual{controller.advance(now, !(flags & 2), flags & 4, flags & 1, 1, target)};
    if(actual != expected) file.fail("MFD child dispatch differs at step " + std::to_string(i));
    compare(static_cast<int>(i));
  }
  file.require_end(); std::cout << count << " original MFD state and dispatch transitions match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
