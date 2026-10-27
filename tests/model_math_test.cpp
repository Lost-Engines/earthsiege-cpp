#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/binary_reader.h"
#include "simulation/model_math.h"

namespace {

template<typename T>
auto vector(earthsiege::binary_reader &file)->vec3<T> {
  return vec3<T>{file.integer<T>(), file.integer<T>(), file.integer<T>()};
}

void require(bool condition, unsigned int row, char const *field) {
  if(!condition) throw std::runtime_error{"native model math mismatch at case " + std::to_string(row) + ": " + field};
}

} // namespace

auto main(int argc, char **argv)->int try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{data};
  unsigned int const count{file.integer<uint32_t>()};
  for(unsigned int row{}; row < count; ++row) {
    auto const angles{vector<uint16_t>(file)};
    auto result{earthsiege::rotation_transform(angles)};
    for(unsigned int const index : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) {
      auto const expected{file.integer<int16_t>()};
      require(result.basis.data[index] == expected, row, "matrix");
    }
    require(result.type == file.integer<uint8_t>(), row, "type");
    file.integer<uint8_t>();
    result.translation = vector<int32_t>(file);
    auto const point{vector<int16_t>(file)};
    auto const transformed{vector<int32_t>(file)};
    require(earthsiege::transform_point(point, result) == transformed, row, "point");
    auto const direction{vector<int32_t>(file)};
    auto const rotated{vector<int32_t>(file)};
    require(earthsiege::rotate_vector(direction, result) == rotated, row, "vector");
    auto const normal{vector<int16_t>(file)};
    auto const distance{file.integer<int32_t>()};
    require(earthsiege::inverse_plane_distance(direction, result, normal) == distance, row, "plane");
  }
  require(file.remaining() == 0, count, "fixture EOF");
  std::cout << count << " original Euler matrices, short points, long vectors and inverse BSP plane tests agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
