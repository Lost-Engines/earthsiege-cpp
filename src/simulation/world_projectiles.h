#pragma once
#include "simulation/sound_request.h"
#include "render/scenery_renderer.h"
#include "simulation/beam_effect.h"
#include "simulation/object_pool.h"

namespace earthsiege {

class resource_store;
inline size_t constexpr projectile_capacity{160};                            // original shared bullet/rocket/beam pool

struct projectile_owner {
  size_t actor{0};
  std::optional<size_t> target;
  int16_t target_component{-1};
};

struct live_projectile {
  projectile_owner owner;
  std::variant<projectile_instance, beam_effect> state;
  live_projectile(projectile_owner owner, beam_effect beam);
  live_projectile(projectile_owner owner, weapon_shot const &shot, weapon_catalogue const &definitions, projectile_motions const &motions, model_resource const &bullets, model_resource const &rockets, int16_t initial_rocket_clock);
};

using world_impact_query = std::function<std::optional<int32_t>(projectile_owner const &, impact_request const &)>;
using projectile_guidance_query = std::function<projectile_guidance(projectile_owner &)>;
using projectile_guidance_feedback = std::function<void(projectile_owner const &, projectile_guidance const &)>;

/// Own every projectile kind in one bounded, ordered pool with deferred reclamation.
class world_projectiles {
  sound_sink sounds;
  weapon_catalogue const &definitions;
  projectile_motions motions;
  model_resource bullet_models;
  model_resource rocket_models;
  std::vector<bitmap> bullet_textures;
  object_pool<live_projectile> projectiles{projectile_capacity};
  // The original missile constructor leaves its clock untouched. Bullets do not
  // overwrite those bytes; beams reset them. Preserve that slot reuse explicitly.
  std::array<int16_t, projectile_capacity> retained_rocket_clocks{};

public:
  world_projectiles(resource_store const &resources, weapon_catalogue const &definitions, sound_sink sounds = {});
  std::optional<size_t> launch(projectile_owner owner, weapon_shot const &shot, random_state &random, world_impact_query const &hit);
  void advance(frame_ratio ratio, projectile_guidance_query const &guidance, world_impact_query const &hit, projectile_guidance_feedback const &feedback = {});
  void collect();
  void append_objects(std::vector<scene_object> &objects);
  object_pool<live_projectile> const &live() const;
};

} // namespace earthsiege
