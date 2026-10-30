#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/texture_raster.h"
#include "resources/binary_reader.h"

auto main(int const argc, char **argv)->int try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESTEXTU2") file.fail("fixture version");
  earthsiege::bitmap texture;
  texture.width = file.integer<uint16_t>();
  texture.height = file.integer<uint16_t>();
  auto const background{file.integer<uint8_t>()};
  auto const cases{file.integer<uint32_t>()};
  auto const pixels{file.take(texture.width * texture.height)};
  texture.pixels.assign(pixels.begin(), pixels.end());
  auto const translation{file.take(256)};
  for(unsigned int row{0}; row != cases; ++row) {
    auto const count{file.integer<uint16_t>()};
    std::vector<earthsiege::projected_vertex> vertices;
    for(unsigned int index{0}; index != count; ++index) vertices.push_back({
      .position{file.integer<int32_t>(), file.integer<int32_t>()},
      .texture{file.integer<int32_t>(), file.integer<int32_t>()},
    });
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    bool const mapped{file.integer<uint8_t>() != 0};
    auto const direction{file.integer<uint8_t>() ? earthsiege::texture_scan_direction::rows : earthsiege::texture_scan_direction::columns};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::framebuffer frame;
    frame.clear(background);
    earthsiege::draw_textured_polygon(frame, vertices, texture, bounds, mapped ? translation : earthsiege::bytes{}, direction);
    if(earthsiege::crc32(frame.pixels()) != expected) throw std::runtime_error{"texture raster differs in original case " + std::to_string(row)};
  }
  file.require_end();
  if(cases == 0) file.fail("empty texture fixture");
  std::cout << cases << " complete native textured-polygon VGA rasters agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
