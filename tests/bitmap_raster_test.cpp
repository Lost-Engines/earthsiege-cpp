#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/bitmap_raster.h"
#include "resources/binary_reader.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  bool const shell{std::string(magic.begin(),magic.end())=="ESSCLGO1"};
  if(!shell&&std::string(magic.begin(), magic.end()) != "ESBITMAP") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  earthsiege::bitmap bitmap;
  bitmap.width = file.integer<uint16_t>();
  bitmap.height = file.integer<uint16_t>();
  auto const background{file.integer<uint8_t>()};
  auto const pixels{file.take(bitmap.width * bitmap.height)};
  bitmap.pixels.assign(pixels.begin(), pixels.end());
  for(unsigned int row{0}; row != count; ++row) {
    vec2<int32_t> const size{file.integer<int32_t>(), file.integer<int32_t>()};
    vec2<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>()};
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::framebuffer frame;
    frame.clear(background);
    (shell?earthsiege::draw_shell_scaled_bitmap:earthsiege::draw_scaled_bitmap)(frame, bitmap, vec2<int32_t>{position}, vec2<int32_t>{size}, bounds);
    if(earthsiege::crc32(frame.pixels()) != expected) throw std::runtime_error{"scaled bitmap differs at " + std::to_string(row)};
  }
  file.require_end();
  if(count != 500) file.fail("incomplete bitmap fixtures");
  std::cout << count << " original scaled bitmaps match all VGA pixels" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
