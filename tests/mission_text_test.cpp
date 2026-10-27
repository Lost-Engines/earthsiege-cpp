#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/mission_text.h"
#include "resources/resource_store.h"

auto main(int argc, char *argv[])->int try {
  if(argc != 3) throw std::runtime_error{"expected assets and original mission-text fixtures"};
  earthsiege::resource_store const resources{argv[1]};
  std::ifstream fixture{argv[2]};
  std::string name;
  std::string state;
  unsigned int section_index{};
  uint32_t expected_crc{};
  unsigned int cases{};
  while(fixture >> name >> section_index >> state >> std::hex >> expected_crc >> std::dec) {
    auto const text{earthsiege::read_mission_text(resources.read("zones", "msn/" + name))};
    auto const &section{text.sections.at(section_index)};
    std::array<int16_t, 100> counters{};
    if(state == "one") counters.fill(1);
    else if(state == "pattern") {
      for(unsigned int index{}; index < counters.size(); ++index) counters[index] = static_cast<int16_t>(index % 5);
    }
    auto const rendered{earthsiege::render_text(std::get<earthsiege::text_program>(section.content), counters)};
    auto const data{earthsiege::bytes{reinterpret_cast<uint8_t const *>(rendered.data()), rendered.size()}};
    if(earthsiege::crc32(data) != expected_crc) throw std::runtime_error{"mission text differs from original interpreter: " + name};
    ++cases;
  }
  if(cases != 2223) throw std::runtime_error{"incomplete original mission text fixtures"};
  std::cout << cases << " original briefing/debrief text results agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
