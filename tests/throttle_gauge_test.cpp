#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "render/instrument_bars.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESTHROT1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int failed{};
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()}; auto const speed{file.integer<int32_t>()}, request{file.integer<int32_t>()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    throttle_gauge gauge{resources, layout};
    auto const thumb{gauge.thumb_bounds(request)};
    for(auto edge : {thumb.left,thumb.top,thumb.right,thumb.bottom}) if(edge != file.integer<int32_t>()) file.fail(name + " thumb position differs");
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame; frame.clear(123); gauge.draw(frame, speed, request);
    if(crc32(frame.pixels()) != expected) { ++failed; if(failed < 16) std::cerr << name << " speed " << speed << " request " << request << " differs: " << crc32(frame.pixels()) << " expected " << expected << '\n'; }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " throttle frames differ"};
  std::cout << count << " original throttle instrument frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
