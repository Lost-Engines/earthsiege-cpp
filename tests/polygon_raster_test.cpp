#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/polygon_raster.h"
#include "resources/binary_reader.h"

namespace {

std::vector<vec2<int32_t>> read_points(earthsiege::binary_reader &file) {
  auto const count{file.integer<uint16_t>()};
  std::vector<vec2<int32_t>> result;
  for(unsigned int i{0}; i != count; ++i) result.push_back({file.integer<int32_t>(), file.integer<int32_t>()});
  return result;
}

} // anonymous namespace

auto main(int const argc, char **argv)->int try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESRASTE2") file.fail("fixture version");
  auto const tables{file.integer<uint32_t>()};
  auto const clips{file.integer<uint32_t>()};
  auto const solid{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != tables; ++row) {
    auto const points{read_points(file)};
    auto const y{file.integer<int16_t>()};
    auto const count{file.integer<uint16_t>()};
    auto const edges{earthsiege::make_polygon_edges(points)};
    if(edges.first_row != y || edges.spans.size() != count) throw std::runtime_error{"edge table bounds differ: " + std::to_string(row)};
    for(unsigned int index{0}; index != count; ++index) {
      auto const left{file.integer<int16_t>()};
      auto const right{file.integer<int16_t>()};
      if(edges.spans[index].left != left || edges.spans[index].right != right) throw std::runtime_error{"edge table differs: " + std::to_string(row) + " span " + std::to_string(index)};
    }
  }
  for(unsigned int row{0}; row != clips; ++row) {
    auto const points{read_points(file)};
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{read_points(file)};
    auto const clipped{earthsiege::clip_polygon_rectangle(points, bounds)};
    if(clipped != expected) throw std::runtime_error{"viewport clip differs: " + std::to_string(row)};
  }
  for(unsigned int row{0}; row != solid; ++row) {
    auto const points{read_points(file)};
    earthsiege::rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const colour{file.integer<uint8_t>()};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::framebuffer frame;
    earthsiege::draw_solid_polygon(frame, points, bounds, colour);
    if(earthsiege::crc32(frame.pixels()) != expected) throw std::runtime_error{"native polygon pixels differ: " + std::to_string(row)};
  }
  file.require_end();
  if(tables != 5999 || clips != 6000 || solid != 360) file.fail("incomplete raster fixture");
  std::cout << tables << " native edge tables and " << clips << " viewport clips and " << solid << " complete VGA rasters agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
