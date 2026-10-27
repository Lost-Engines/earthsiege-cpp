#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/line_raster.h"
#include "resources/binary_reader.h"

namespace {

vec2<int32_t> point(earthsiege::binary_reader &file) {
  return {file.integer<int32_t>(), file.integer<int32_t>()};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESLINES1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const colour{file.integer<uint8_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    auto const first{point(file)};
    auto const second{point(file)};
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    bool const visible{file.integer<uint8_t>() != 0};
    auto const clipped_first{point(file)};
    auto const clipped_second{point(file)};
    auto const expected{file.integer<uint32_t>()};
    auto a{first};
    auto b{second};
    if(earthsiege::clip_line_rectangle(a, b, bounds) != visible || (visible && (a != clipped_first || b != clipped_second))) throw std::runtime_error{"line clip differs at " + std::to_string(row)};
    earthsiege::framebuffer frame;
    earthsiege::draw_model_line(frame, vec2<int32_t>{first}, vec2<int32_t>{second}, bounds, colour);
    if(earthsiege::crc32(frame.pixels()) != expected) throw std::runtime_error{"line pixels differ at " + std::to_string(row)};
  }
  file.require_end();
  if(count != 6000) file.fail("incomplete line fixtures");
  std::cout << count << " native line clips and whole VGA rasters agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
