#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/briefing_map.h"
#include "resources/resource_store.h"
int main(int argc, char **argv) try {
  using earthsiege::binary_reader;
  using earthsiege::briefing_map;
  using earthsiege::crc32;
  using earthsiege::framebuffer;
  using earthsiege::read_file;
  using earthsiege::read_mission_script;
  using earthsiege::resource_store;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto raw{read_file(argv[2])};
  binary_reader f{raw};
  auto magic{f.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESBRLAB1") f.fail("fixture version");
  auto cases{f.integer<uint32_t>()};
  unsigned frames{0};
  unsigned failures{0};
  for(unsigned c{0}; c < cases; ++c) {
    auto script{read_mission_script(f.take(f.integer<uint32_t>()))};
    auto players{f.integer<uint16_t>()};
    auto texts{f.integer<uint16_t>()};
    std::vector<std::string> text;
    for(unsigned j{0}; j < texts; ++j) text.push_back(f.cstring());
    briefing_map map{resources, script, players, text};
    auto count{f.integer<uint32_t>()};
    for(unsigned i{0}; i < count; ++i) {
      auto action{f.integer<uint32_t>()};
      int x{f.integer<int32_t>()};
      int y{f.integer<int32_t>()};
      auto crc{f.integer<uint32_t>()};
      if(action) map.navigate(static_cast<uint16_t>(action));
      map.hover(x, y);
      framebuffer frame;
      frame.clear(37);
      map.draw(frame);
      ++frames;
      if(crc32(frame.pixels()) != crc) {
        if(!failures) {
          std::ofstream out{"/tmp/es-briefing-label-difference.raw", std::ios::binary};
          out.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
        }
        ++failures;
        std::cerr << "category " << c << " label " << i << " differs\n";
      }
    }
  }
  f.require_end();
  if(failures) f.fail(std::to_string(failures) + " map labels differ");
  std::cout << frames << " original map-label frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
