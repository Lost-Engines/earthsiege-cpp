#pragma once
#include <optional>
#include "render/scenery_renderer.h"
#include "resources/interface_strings.h"

namespace earthsiege {

struct missile_camera_object {
  size_t id;
  vec3<int32_t> position;
  vec3<uint16_t> angles;
};
struct missile_camera_explosion { size_t id; vec3<int32_t> position; };
enum class missile_picture : uint8_t { idle, camera, flash };
struct missile_camera_frame {
  missile_picture picture{missile_picture::idle};
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  bool bright{};
};

/// Retain the original pool-slot references and last camera pose through impact.
class missile_camera_tracker {
  std::optional<size_t> projectile;
  std::optional<size_t> explosion;
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  bool bright{};
public:
  void watch(std::optional<size_t> slot);
  std::optional<size_t> watched() const;
  std::optional<size_t> flashing_explosion() const;
  missile_camera_frame advance(std::span<missile_camera_object const> missiles, std::span<missile_camera_explosion const> explosions);
};

class missile_display {
  framebuffer &target;
  scenery_renderer scenery;
  std::array<font, 3> fonts;                                                 // red, blue, yellow
  interface_strings strings;
public:
  missile_display(resource_store const &resources, framebuffer &target, scenery_world const &world);
  void draw(rectangle const &bounds, missile_camera_frame const &frame, bool ready, bool locked, uint32_t ticks,
            std::span<light const> lights, std::span<scene_object const> objects = {}, effect_lighting *dynamic = nullptr,
            std::span<ground_flash const> ground = {});
};

} // namespace earthsiege
