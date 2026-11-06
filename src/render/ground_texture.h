#pragma once
#include "render/panorama.h"
#include <optional>
#include "vectorstorm/vector/vector3.h"
namespace earthsiege {
/// Original cached scanline projection and 24-bit wrapping ground sampler.
class ground_texture {
  std::vector<uint8_t> source;
  std::vector<int32_t> thresholds;
  std::array<std::array<uint8_t,16>,15> shades;
  std::array<std::array<int32_t,3>,320> lines{};
  int32_t cached_height{-1},cached_pitch{-1},cached_count{},cached_offset{};
  int32_t stride{320};
  int32_t fog_rows{};
  std::vector<uint8_t> expansion_scratch;
  bool expansion{};
  unsigned texture_scale{8};
  void draw_expansion(framebuffer &,vec3<int32_t> const &,vec3<uint16_t> const &,background_view const &,background_bands const &);
  bitmap scratch;
  std::optional<panorama_placement> last;
public:
  ground_texture(world_environment const &,bytes texture,bytes distances);
  void draw(framebuffer &,vec3<int32_t> const &position,vec3<uint16_t> const &angles,
            background_view const &,background_bands const &);
  bitmap const &image() const { return scratch; }
  auto const &placement() const { return last; }
};
}
