#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "render/target_instruments.h"
#include "resources/resource_store.h"
#include "simulation/model_math.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 2) return EXIT_FAILURE;
  auto const source{read_file(argv[1])}; binary_reader file{source}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHUDT01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned int i{}; i < count; ++i) {
    camera_projection projection; projection.centre={file.integer<int32_t>(),file.integer<int32_t>()};
    vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
    hud_target_state state;
    state.position={file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    state.model_radius=file.integer<int32_t>(); state.locked=file.integer<uint8_t>()!=0; state.side=file.integer<uint8_t>();
    bool const present{file.integer<uint8_t>()!=0}; state.visible=file.integer<uint8_t>()!=0;
    rectangle const expected{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    auto const raster{file.integer<uint32_t>()};
    auto camera{rotation_transform(angles)}; camera.translation=position;
    auto const bounds{present ? target_brackets(state,inverse_transform(camera),projection) : std::nullopt};
    auto const actual{bounds.value_or(rectangle{})};
    if(actual.left!=expected.left || actual.top!=expected.top || actual.right!=expected.right || actual.bottom!=expected.bottom)
      file.fail("target bounds differ at "+std::to_string(i));
    framebuffer frame; frame.clear(123);
    if(bounds) draw_target_brackets(frame,*bounds,state.locked,state.side);
    if(crc32(frame.pixels())!=raster) {
      std::ofstream output{"/tmp/es-target-difference.raw",std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()),frame.pixels().size());
      file.fail("target pixels differ at "+std::to_string(i)+": "+std::to_string(crc32(frame.pixels()))+" expected "+std::to_string(raster));
    }
  }
  file.require_end();std::cout<<count<<" original target bracket frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
