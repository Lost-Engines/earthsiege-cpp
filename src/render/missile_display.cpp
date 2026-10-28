#include "missile_display.h"
#include <algorithm>
#include "render/background_bands.h"
#include "resources/resource_store.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
void missile_camera_tracker::watch(std::optional<size_t> const slot) { projectile = slot; }
std::optional<size_t> missile_camera_tracker::watched() const { return projectile; }
std::optional<size_t> missile_camera_tracker::flashing_explosion() const { return explosion; }
missile_camera_frame missile_camera_tracker::advance(std::span<missile_camera_object const> missiles, std::span<missile_camera_explosion const> explosions) {
  auto const result{[&](missile_picture const picture){ return missile_camera_frame{picture, vec3<int32_t>{position}, vec3<uint16_t>{angles}, bright}; }};
  if(explosion) {
    if(std::ranges::any_of(explosions, [&](auto const &object){ return object.id == *explosion; })) {
      bright = !bright;
      return result(missile_picture::flash);
    }
    explosion.reset();
  }
  if(projectile) {
    auto const found{std::ranges::find(missiles, *projectile, &missile_camera_object::id)};
    if(found == missiles.end()) projectile.reset();
    else {
      position = found->position; angles = found->angles;
      position.x = wrap_long(static_cast<int64_t>(position.x) + ((static_cast<int32_t>(model_cosine(angles.z)) * 500 + 8192) >> 14));
      position.y = wrap_long(static_cast<int64_t>(position.y) + ((static_cast<int32_t>(model_cosine(static_cast<uint16_t>(angles.z - angle_quarter_turn))) * 500 + 8192) >> 14));
    }
    for(auto const &object : explosions) {
      vec3<int32_t> delta;
      for(unsigned int axis{}; axis < 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(position[axis]) - object.position[axis]);
      if(fast_distance(delta) < 10000) {
        explosion = object.id; bright = true;
        return result(missile_picture::flash);
      }
    }
    if(projectile) return result(missile_picture::camera);
  }
  return result(missile_picture::idle);
}

missile_display::missile_display(resource_store const &resources, framebuffer &buffer, scenery_world const &world)
  : target{buffer}, scenery{buffer, world, read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT")), scene_shading::cockpit_map},
    fonts{read_font(resources.read("simvol0", "dfn/CPRED.DFN")), read_font(resources.read("simvol0", "dfn/CPBLUE.DFN")), read_font(resources.read("simvol0", "dfn/CPYLW.DFN"))},
    strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))} {}

void missile_display::draw(rectangle const &bounds, missile_camera_frame const &frame, bool const ready, bool const locked, uint32_t const ticks,
                           std::span<light const> lights, std::span<scene_object const> objects, effect_lighting * const dynamic, std::span<ground_flash const> ground) {
  scoped_framebuffer_clip unclipped{target, nullptr};
  if(frame.picture == missile_picture::flash) { target.fill(bounds, frame.bright ? 60 : 63); return; }
  if(frame.picture == missile_picture::idle) {
    target.fill(bounds, 63);
    auto const text{[&](size_t const index)->std::string const &{ return strings.at(31).at(index + 1).text; }};
    auto const label{[&](size_t const face, std::string const &value, int const y){
      target.text(fonts[face], value, bounds.left + 34 - ((fonts[face].text_width(value) - 1) >> 1), bounds.top + y);
    }};
    if(ready) label(0, text(0), 10);
    label(0, text(ready ? 1 : 2), 17);
    target.fill({bounds.left + 22, bounds.top + 32, bounds.left + 46, bounds.top + 42}, locked ? (ticks & 32 ? 53 : 63) : 48);
    label(locked ? 2 : 1, text(3), 41);
    return;
  }
  screen_mask mask;
  screen_region region; region.first_row = bounds.top; region.rectangular = true;
  region.spans.resize(static_cast<size_t>(bounds.bottom - bounds.top + 1), {bounds.left, bounds.right});
  mask.regions.push_back(std::move(region)); mask.translate(0, 0);
  scoped_framebuffer_clip clipped{target, &mask};
  model_view view;
  view.bounds = bounds;
  view.projection.perspective_shift = 7; view.projection.near_distance = 128;
  view.projection.centre = {bounds.left + ((bounds.right - bounds.left) >> 1), bounds.top + ((bounds.bottom - bounds.top) >> 1)};
  world_environment environment;
  environment.background = {1, 16, 1, 3, 22, 1, 3, 0};
  background_view const background{.pitch{frame.angles.x}, .roll{frame.angles.y}, .projection_shift{7}, .centre{vec2<int32_t>{view.projection.centre}}, .offset{0, 0}, .bounds{bounds}};
  draw_background_bands(target, make_background_bands(environment, background), bounds);
  scenery.draw_objects(frame.position, frame.angles, lights, false, objects, dynamic, ground, view);
}
} // namespace earthsiege
