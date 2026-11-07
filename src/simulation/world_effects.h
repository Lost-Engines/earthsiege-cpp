#pragma once
#include "simulation/sound_request.h"
#include <functional>
#include "render/scenery_renderer.h"
#include "resources/model.h"
#include "simulation/effect_state.h"
#include "simulation/deployment.h"
#include "simulation/model_instance.h"
#include "simulation/object_pool.h"

namespace earthsiege {

class resource_store;
inline size_t constexpr explosion_capacity{40};
inline size_t constexpr debris_capacity{150};
inline size_t constexpr smoke_puff_capacity{200};
inline size_t constexpr smoke_emitter_capacity{20};
inline size_t constexpr effect_light_capacity{3};                              // controller pool, distinct from the lighting manager's twenty sources

struct live_explosion {
  int16_t definition;
  explosion_state state;
  model_instance model;
  std::optional<model_instance> secondary;
  std::optional<size_t> light_slot;
  vec3<int32_t> secondary_position;
  live_explosion(int16_t definition, explosion_state state, shape_part const &shape, shape_part const &secondary_shape);
  void synchronize_cells();
};

struct live_debris {
  debris_particle state;
  model_instance model;
  bool detached_weapon{false};
  live_debris(debris_particle state, shape_part const &shape, bool weapon = false);
};
struct live_smoke_puff {
  smoke_puff_state state;
  model_instance model;
  live_smoke_puff(smoke_puff_state state, shape_part const &shape);
};
struct live_smoke_emitter {
  smoke_emitter_state state;
  std::function<vec3<int32_t>()> attachment;
};

struct live_deployment {
  deployment_state state;std::optional<size_t> group;model_instance body,opening;
  live_deployment(deployment_state s,std::optional<size_t> g,shape_part const &a,shape_part const &b):state{s},group{g},body{a},opening{b}{}
};

/// Own live effect geometry and state; phase-five collection remains explicit.
/// Attachments resolve their owner at emission time, before actor movement.
class world_effects {
  sound_sink sounds;
  std::vector<explosion_definition> explosion_definitions;
  std::vector<debris_definition> debris_definitions;
  model_resource explosion_models;
  model_resource debris_models;
  model_resource deployment_models;
  std::vector<bitmap> deployment_textures;
  object_pool<live_deployment> deployments{10};
  std::vector<vec3<int32_t>> craters;
  model_resource flat_models;
  std::vector<model_instance> flat_instances;
  model_resource smoke_models;
  model_resource detached_weapon_models;
  std::vector<bitmap> smoke_textures;
  std::vector<bitmap> weapon_textures;
  std::vector<bitmap> flat_textures;
  std::vector<std::vector<bitmap>> explosion_textures;
  effect_lighting lighting{std::array{default_world_light()}, 2000, 10};
  object_pool<size_t> light_owners{effect_light_capacity};
  object_pool<live_explosion> explosions{explosion_capacity};
  object_pool<live_debris> debris{debris_capacity};
  object_pool<live_smoke_puff> puffs{smoke_puff_capacity};
  object_pool<live_smoke_emitter> emitters{smoke_emitter_capacity};
  std::array<vec3<int16_t>, smoke_puff_capacity> retained_puff_velocities{};

public:
  explicit world_effects(resource_store const &resources, sound_sink sounds = {}, bool alternate_ground_models = false);
  std::optional<size_t> add_explosion(int16_t definition, vec3<int32_t> const &position, random_state &random);
  void add_debris(int16_t group, vec3<int32_t> const &position, random_state &random);
  std::optional<size_t> add_smoke(int16_t family, std::function<vec3<int32_t>()> attachment, random_state &visual_random);
  std::optional<size_t> add_puff(int16_t root, vec3<int32_t> const &position);
  std::optional<size_t> add_detached_weapon(debris_particle state);
  void advance(frame_ratio ratio);
  bool add_deployment(std::optional<size_t> group,vec3<int32_t> const &destination,random_state &random);
  void advance_deployments(frame_ratio ratio,std::function<bool(vec3<int32_t> const &)> const &impact,std::function<void(size_t,vec3<int32_t> const &)> const &release,vec3<int32_t> const &listener);
  void collect();
  effect_lighting &lights();
  /// Draw views stay valid until the next simulation update or collection.
  void append_models(std::vector<scene_model> &models);
  std::vector<ground_flash> ground_flashes();
  ground_flash ground_attachment(size_t shape,vec3<int32_t> &position,uint16_t heading);
  object_pool<live_smoke_puff> const &live_smoke() const;
  object_pool<live_smoke_emitter> const &smoke_controllers() const;
  object_pool<live_explosion> const &live_explosions() const;
  object_pool<live_debris> const &live_particles() const;
};

} // namespace earthsiege
