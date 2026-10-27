#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "render/scene_order.h"
#include "resources/binary_reader.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESORDER1") file.fail("fixture version");
  auto const cases{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != cases; ++row) {
    auto const eye{point(file)};
    auto const count{file.integer<uint16_t>()};
    std::vector<earthsiege::scene_order_entry> entries;
    for(unsigned int index{0}; index != count; ++index) {
      auto const position{point(file)};
      auto const distance{file.integer<int32_t>()};
      bool const segment{file.integer<uint8_t>() != 0};
      earthsiege::scene_sort_segment const endpoints{.first{point(file)}, .second{point(file)}};
      entries.push_back(earthsiege::scene_order_entry{.position{vec3<int32_t>{position}}, .distance{distance}, .segment{segment ? std::optional{endpoints} : std::nullopt}});
    }
    auto const actual{earthsiege::scene_painter_order(entries, eye)};
    if(actual.size() != count) throw std::runtime_error{"scene order lost objects"};
    for(auto const index : actual) if(index != file.integer<uint16_t>()) throw std::runtime_error{"scene painter order differs in case " + std::to_string(row)};
  }
  file.require_end();
  if(cases != 2000) file.fail("incomplete scene order fixture");
  std::cout << cases << " original whole-scene painter trees agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
