#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include "render/instrument_bars.h"
#include "render/shield_instruments.h"
#include "shell/pointer_controls.h"
#include "simulation/shield_power.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPTR002") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; unsigned int failed{}, total{};
  for(unsigned int row{}; row < count; ++row) {
    auto const name{file.cstring()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    throttle_gauge throttle{resources, layout}; shield_instruments shields{resources, layout}; pointer_controls pointer;
    pointer.add_slider(throttle.slider_layout(), 0);
    for(auto const &button : shields.button_bounds()) pointer.add_button(button);
    int request{}; shield_power shield{.front{5000}, .rear{5000}, .balance{512}, .capacity{10000}};
    auto const cases{file.integer<uint32_t>()};
    for(unsigned int i{}; i < cases; ++i) {
      auto const ticks{file.integer<uint32_t>()};bool const double_click{file.integer<uint8_t>()!=0};
      auto const x{file.integer<int32_t>()}, y{file.integer<int32_t>()}; auto const buttons{file.integer<uint8_t>()};
      if(auto const action{pointer.event(x, y, (buttons & 1) != 0, (buttons & 2) != 0,ticks)}) {
        if(action->index == 0) request = throttle.slider_layout().vertical ? -action->value : action->value;
        else if(action->value & 1) change_shield_balance(shield, action->index == 1);
      }
      pointer.painted(0);
      bool matches{pointer.is_double_click()==double_click};
      auto const check{[&](auto actual, auto expected){ if(actual != expected) matches = false; }};
      check(pointer.active() ? static_cast<int>(*pointer.active()) : -1, file.integer<int16_t>());
      check(pointer.is_dragging(), file.integer<uint8_t>() != 0);
      for(size_t j{}; j < 3; ++j) check(std::to_underlying(pointer.at(j).face), file.integer<uint8_t>());
      auto const thumb{pointer.at(0).painted_thumb};
      for(auto const v : {thumb.left, thumb.top, thumb.right, thumb.bottom}) check(v, file.integer<int32_t>());
      check(pointer.at(0).boundary, file.integer<int32_t>()); check(request, file.integer<int32_t>()); check(shield.balance, file.integer<int32_t>());
      if(!matches) { ++failed; if(failed < 15) std::cerr << name << " pointer cycle " << i << " differs\n"; }
      ++total;
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " pointer cycles differ"};
  std::cout << total << " original cockpit pointer cycles match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
