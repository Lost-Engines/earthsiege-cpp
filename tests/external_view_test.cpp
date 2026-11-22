#include <cstdlib>
#include <iostream>
#include "render/external_view.h"
int main(int argc, char **argv) try {
  using earthsiege::binary_reader;
  using earthsiege::external_view_artwork;
  using earthsiege::framebuffer;
  using earthsiege::read_file;
  using earthsiege::resource_store;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  external_view_artwork artwork{resources};
  auto raw{read_file(argv[2])};
  binary_reader file{raw};
  auto signature{file.take(8)};
  if(std::string(signature.begin(), signature.end()) != "ESEXT001") file.fail("format");
  auto const count{file.integer<uint32_t>()};
  for(uint32_t i{0}; i < count; ++i) {
    auto const control{file.integer<uint8_t>()};
    auto expected{file.take(320 * 29)};
    framebuffer frame;
    frame.clear();
    artwork.draw(frame, control != 0);
    size_t errors{0};
    for(size_t p{0}; p < expected.size(); ++p) {
      if(expected[p] != frame.pixels()[320 * 171 + p]) ++errors;
    }
    if(errors) throw std::runtime_error{"external control " + std::to_string(control) + ": " + std::to_string(errors) + " different pixels"};
  }
  file.require_end();
  std::cout << count << " original external status rasters agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
