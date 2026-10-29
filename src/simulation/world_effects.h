#pragma once
#include "simulation/sound_request.h"
#include "render/scenery_renderer.h"
#include "resources/model.h"
#include "simulation/effect_state.h"
#include "simulation/model_instance.h"
#include "simulation/object_pool.h"

namespace earthsiege {

class resource_store;
inline size_t constexpr explosion_capacity{40};
inline size_t constexpr debris_capacity{150};
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
  live_debris(debris_particle state, shape_part const &shape);
};

/// Own live effect geometry and state; phase-five collection remains explicit.
/// Attached smoke controllers and sound playback remain separate consumers.
class world_effects {
  sound_sink sounds;
  std::vector<explosion_definition> explosion_definitions;
  std::vector<debris_definition> debris_definitions;
  model_resource explosion_models;
  model_resource debris_models;
  model_resource flat_models;
  std::array<std::vector<bitmap>, 9> explosion_textures;
  effect_lighting lighting{std::array{default_world_light()}, 2000, 10};
  object_pool<size_t> light_owners{effect_light_capacity};
  object_pool<live_explosion> explosions{explosion_capacity};
  object_pool<live_debris> debris{debris_capacity};

public:
  explicit world_effects(resource_store const &resources, sound_sink sounds = {});
  std::optional<size_t> add_explosion(int16_t definition, vec3<int32_t> const &position, random_state &random);
  void add_debris(int16_t group, vec3<int32_t> const &position, random_state &random);
  void advance(frame_ratio ratio);
  void collect();
  effect_lighting &lights();
  /// Draw views stay valid until the next simulation update or collection.
  void append_models(std::vector<scene_model> &models);
  std::vector<ground_flash> ground_flashes();
  object_pool<live_explosion> const &live_explosions() const;
  object_pool<live_debris> const &live_particles() const;
};

} // namespace earthsiege
