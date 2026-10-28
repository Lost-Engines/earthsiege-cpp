#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "render/presentation_player.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 4) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const raw{read_file(argv[2])}; binary_reader programs{raw}; auto const magic{programs.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESTTMP01") programs.fail("fixture version");
  auto const program_count{programs.integer<uint32_t>()};
  for(unsigned int i{}; i < program_count; ++i) {
    auto const name{programs.cstring()}; auto const separator{name.find('/')};
    auto const pages{programs.integer<uint16_t>()}, labels{programs.integer<uint16_t>()};
    auto const amount{programs.integer<uint32_t>()}; auto const expected{programs.take(amount)};
    auto const program{read_presentation(resources.read(name.substr(0, separator), name.substr(separator + 1)))};
    if(program.pages.size() != pages || program.labels.size() != labels || !std::ranges::equal(program.bytecode, expected)) programs.fail(name + " program differs");
    for(auto const &label : program.labels) {
      auto const number{programs.integer<uint16_t>()}; auto const text{programs.cstring()};
      if(label.number != number || label.name != text) programs.fail(name + " label differs");
    }
  }
  programs.require_end();
  auto const source{read_file(argv[3])}; binary_reader file{source}; auto const signature{file.take(8)};
  if(std::string(signature.begin(), signature.end()) != "ESTTM001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis; std::unique_ptr<presentation_player> player;
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()};
    if(chassis != name) { chassis = name; player = std::make_unique<presentation_player>(resources, "simvol0", "ttm/" + name + ".TTM", 12345); }
    auto const elapsed{file.integer<int16_t>()}, label{file.integer<int16_t>()};
    auto const page{file.integer<uint8_t>()}; auto const mode{file.integer<int32_t>()}; bool const latch{file.integer<uint8_t>() != 0};
    auto const remaining{file.integer<int16_t>()}, period{file.integer<int16_t>()}, jump{file.integer<int16_t>()};
    auto const random{file.integer<uint32_t>()}, expected{file.integer<uint32_t>()};
    if(label) player->select_label(label);
    player->advance(elapsed); auto const &state{player->state()};
    if(state.page != page || state.mode != mode || state.wait_latched != latch || state.remaining != remaining || state.period != period || state.jump != jump || state.random != random)
      file.fail(name + " animation state differs at " + std::to_string(i));
    framebuffer frame; frame.clear(123); player->draw(frame);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-ttm-difference.raw", std::ios::binary}; output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail(name + " animation paint differs at " + std::to_string(i) + " CRC " + std::to_string(crc32(frame.pixels())) + " expected " + std::to_string(expected));
    }
  }
  file.require_end(); std::cout << program_count << " TTM programs and " << count << " original animation frames match\n"; return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
