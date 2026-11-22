#include <cstdlib>
#include <iostream>
#include "render/cockpit_pan.h"
#include "resources/binary_reader.h"
using earthsiege::binary_reader;
using earthsiege::cockpit_surface;
using earthsiege::crc32;
using earthsiege::read_file;
using earthsiege::rectangle;
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto data{read_file(argv[1])};
  binary_reader f{data};
  auto magic{f.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESVCOPY1") f.fail("copy fixture version");
  auto cases{f.integer<uint32_t>()};
  auto source{f.take(640 * 400)};
  for(unsigned i{0}; i < cases; ++i) {
    auto sx{f.integer<int32_t>()};
    auto dx{f.integer<int32_t>()};
    rectangle bounds{f.integer<int32_t>(), f.integer<int32_t>(), f.integer<int32_t>(), f.integer<int32_t>()};
    auto expected{f.integer<uint32_t>()};
    cockpit_surface surface;
    surface.restore(source);
    surface.copy_page(sx, dx, bounds);
    if(crc32(surface.pixels()) != expected) f.fail("original planar copy " + std::to_string(i));
  }
  f.require_end();
  std::cout << cases << " original page copies, including external-return wrap, agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
