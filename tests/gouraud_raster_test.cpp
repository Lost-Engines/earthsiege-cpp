#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/gouraud_raster.h"
#include "resources/binary_reader.h"

namespace {

std::array<vec2<int32_t>, 3> read_points(earthsiege::binary_reader &file) {
  std::array<vec2<int32_t>, 3> result;
  for(auto &point : result) point = vec2<int32_t>{file.integer<int32_t>(), file.integer<int32_t>()};
  return result;
}

std::array<uint8_t, 3> read_colours(earthsiege::binary_reader &file) {
  return {file.integer<uint8_t>(), file.integer<uint8_t>(), file.integer<uint8_t>()};
}

} // anonymous namespace

auto main(int const argc, char **argv)->int try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESGOURA1") file.fail("fixture version");
  auto const emissions{file.integer<uint32_t>()};
  auto const rasters{file.integer<uint32_t>()};
  auto const background{file.integer<uint8_t>()};
  for(unsigned int row{0}; row != emissions; ++row) {
    auto const points{read_points(file)};
    auto const colours{read_colours(file)};
    auto const count{file.integer<uint16_t>()};
    auto const expected{file.integer<uint32_t>()};
    auto const bands{earthsiege::split_gouraud_triangle(points, colours)};
    std::vector<uint8_t> bytes;
    for(auto const &band : bands) {
      bytes.push_back(band.vertex_count);
      bytes.push_back(band.colour);
      for(unsigned int index{0}; index != band.vertex_count; ++index) {
        for(unsigned int axis{0}; axis != 2; ++axis) {
          auto const value{static_cast<uint32_t>(band.points[index][axis])};
          for(unsigned int byte{0}; byte != 4; ++byte) bytes.push_back(static_cast<uint8_t>(value >> (byte * 8)));
        }
      }
    }
    if(bands.size() != count || earthsiege::crc32(bytes) != expected) throw std::runtime_error{"Gouraud bands differ in original case " + std::to_string(row)};
  }
  for(unsigned int row{0}; row != rasters; ++row) {
    auto const points{read_points(file)};
    auto const colours{read_colours(file)};
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::framebuffer frame;
    frame.clear(background);
    earthsiege::draw_gouraud_triangle(frame, points, colours, bounds);
    if(earthsiege::crc32(frame.pixels()) != expected) throw std::runtime_error{"Gouraud pixels differ in original case " + std::to_string(row)};
  }
  file.require_end();
  if(emissions != 6000 || rasters != 300) file.fail("incomplete Gouraud fixture");
  std::cout << emissions << " original Gouraud band emissions and " << rasters << " complete VGA rasters agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
