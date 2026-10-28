#include <cstdlib>
#include <iostream>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/controller.h"

namespace {

struct state_record {
  earthsiege::controller_state controller;
  bool skip{false};
  uint16_t stagger{0};
};
state_record state(earthsiege::binary_reader &file) {
  state_record result;
  result.controller.type = static_cast<earthsiege::behavior>(file.integer<uint8_t>());
  result.controller.reconsider_remaining = file.integer<int32_t>(); result.controller.ticks = file.integer<uint32_t>();
  result.controller.updated = file.integer<uint8_t>(); result.controller.first_entry = file.integer<uint8_t>();
  result.skip = file.integer<uint8_t>(); result.stagger = file.integer<uint16_t>();
  return result;
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCTRL01") file.fail("fixture version");
  auto const tables{file.integer<uint8_t>()};
  for(unsigned int i{0}; i != tables; ++i) {
    auto const name{file.take(file.integer<uint8_t>())};
    auto const &definition{earthsiege::describe(static_cast<earthsiege::behavior>(i))};
    auto const period{file.integer<int32_t>()}; auto const properties{file.integer<uint16_t>()}; auto const status{file.integer<int16_t>()};
    if(definition.name != std::string(name.begin(),name.end()) || definition.reconsider_period != period || definition.properties != properties || definition.status.value_or(-1) != status) file.fail("controller table");
  }
  auto const entries{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != entries; ++i) {
    auto const type{static_cast<earthsiege::behavior>(file.integer<uint8_t>())}; auto stagger{file.integer<uint16_t>()};
    auto const expected_stagger{static_cast<uint16_t>(stagger + 13)};
    earthsiege::controller_state value;
    value.target_search_remaining = 123; value.ticks = 123;
    earthsiege::enter_behavior(value,type,stagger);
    if(value.target_search_remaining || value.ticks || value.updated || !value.first_entry || value.type != type || stagger != expected_stagger || value.reconsider_remaining != earthsiege::describe(type).reconsider_period + (stagger & 15)) file.fail("controller entry");
    for(unsigned int tick{0}; tick != 3; ++tick) {
      earthsiege::finish_controller_update(value);
      auto const ticks{file.integer<uint32_t>()}; bool const updated{file.integer<uint8_t>() != 0}, first{file.integer<uint8_t>() != 0};
      if(value.ticks != ticks || value.updated != updated || value.first_entry != first) file.fail("controller first/update latch");
    }
  }
  auto const selections{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != selections; ++i) {
    auto const kind{file.integer<uint8_t>()}; earthsiege::behavior selected;
    if(kind == 0) {
      bool const player{file.integer<uint8_t>() != 0}; auto const order{file.integer<int16_t>()}, limit{file.integer<int16_t>()}; file.integer<uint8_t>();
      selected = earthsiege::initial_herc_behavior(player,order,limit);
    } else selected = earthsiege::initial_flyer_behavior(file.integer<int16_t>());
    if(selected != static_cast<earthsiege::behavior>(file.integer<uint8_t>())) file.fail("controller selection");
  }
  auto const statuses{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != statuses; ++i) {
    auto const type{static_cast<earthsiege::behavior>(file.integer<uint8_t>())}; bool const disabled{file.integer<uint8_t>() != 0}, dead{file.integer<uint8_t>() != 0};
    auto const command{file.integer<int16_t>()}, expected{file.integer<int16_t>()};
    if(earthsiege::herc_status(type,disabled,dead,command) != expected) file.fail("controller status");
  }
  auto const dispatches{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != dispatches; ++i) {
    auto input{state(file)}; auto const ratio{file.integer<int16_t>()};
    auto const reconsider{file.integer<int8_t>()}, update{file.integer<int8_t>()};
    bool const skip_update{file.integer<uint8_t>() != 0}, movement_result{file.integer<uint8_t>() != 0};
    auto const expected{state(file)};
    std::vector<std::pair<uint8_t,uint8_t>> events;
    earthsiege::dispatch_controller(input.controller,input.skip,ratio,{
      .reconsider{[&](earthsiege::behavior type){ events.emplace_back(0,static_cast<uint8_t>(type)); if(reconsider >= 0) earthsiege::enter_behavior(input.controller,static_cast<earthsiege::behavior>(reconsider),input.stagger); }},
      .advance{[&](earthsiege::behavior type){ events.emplace_back(1,static_cast<uint8_t>(type)); if(update >= 0) earthsiege::enter_behavior(input.controller,static_cast<earthsiege::behavior>(update),input.stagger); if(skip_update) input.skip = true; }},
      .decide{[&](earthsiege::behavior type){ events.emplace_back(2,static_cast<uint8_t>(type)); return movement_result; }},
    });
    auto const count{file.integer<uint8_t>()};
    if(events.size() != count) file.fail("dispatch event count " + std::to_string(i));
    for(auto const &[phase,type] : events) if(phase != file.integer<uint8_t>() || type != file.integer<uint8_t>()) file.fail("dispatch event order " + std::to_string(i));
    auto const &a{input.controller}; auto const &b{expected.controller};
    if(a.type != b.type || a.reconsider_remaining != b.reconsider_remaining || a.ticks != b.ticks || a.updated != b.updated || a.first_entry != b.first_entry || input.skip != expected.skip || input.stagger != expected.stagger) { std::cerr << "type " << static_cast<int>(*a.type) << "/" << static_cast<int>(*b.type) << " clock " << a.reconsider_remaining << "/" << b.reconsider_remaining << " ticks " << a.ticks << "/" << b.ticks << " flags " << a.updated << a.first_entry << input.skip << "/" << b.updated << b.first_entry << expected.skip << " stagger " << input.stagger << "/" << expected.stagger << '\n'; file.fail("dispatch final state " + std::to_string(i)); }
  }
  file.require_end();
  std::cout << static_cast<unsigned int>(tables) << " controller definitions, " << entries << " entries, " << selections << " selections, " << statuses << " statuses and " << dispatches << " original dispatches agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n'; return EXIT_FAILURE;
}
