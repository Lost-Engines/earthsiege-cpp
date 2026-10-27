#pragma once
#include <span>
#include <vector>
#include "resources/artwork.h"
#include "resources/world.h"
#include "simulation/model_instance.h"
#include "simulation/scenery_collision.h"

namespace earthsiege {

class resource_store;

struct scenery_object {
  size_t shape{0};
  model_transform placement;
  model_instance model;
  terrain_metadata const *metadata{nullptr};
};

/// Own the static models and instances selected by one compiled mission
class scenery_world {
  world_environment environment_data;
  palette palette_data;
  std::vector<bitmap> textures_data;
  model_resource terrain_data;
  std::vector<terrain_metadata> metadata_data;
  std::vector<scenery_object> instances;
  std::vector<scenery_collider> collisions;

public:
  scenery_world(resource_store const &resources, mission_header const &header);
  scenery_world(scenery_world const &) = delete;
  scenery_world &operator=(scenery_world const &) = delete;
  scenery_world(scenery_world &&) = default;
  scenery_world &operator=(scenery_world &&) = default;
  world_environment const &environment() const;
  palette const &colours() const;
  std::span<bitmap const> textures() const;
  model_resource const &terrain() const;
  std::span<scenery_object const> objects() const;
  std::span<scenery_collider const> colliders() const;
};

} // namespace earthsiege
