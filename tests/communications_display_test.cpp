#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/communications_display.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; communications_display display{resources};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCOMM01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(uint32_t i{}; i < count; ++i) {
    rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const disabled{file.integer<uint8_t>()}, alternates{file.integer<uint8_t>()};
    communications_display_state state; state.selected = file.integer<uint8_t>();
    for(size_t j{}; j < state.choices.size(); ++j) state.choices[j] = {bool(disabled & (1U << j)), bool(alternates & (1U << j))};
    if(state.command() != file.integer<uint8_t>()) file.fail("communications command differs");
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame; frame.clear(123); display.draw(frame, bounds, state);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-communications-difference.raw", std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail("communications display paint differs at " + std::to_string(i));
    }
  }
  file.require_end(); std::cout << count << " original communications frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
