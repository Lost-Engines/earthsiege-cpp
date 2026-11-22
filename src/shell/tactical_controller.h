#pragma once
#include "render/tactical_map.h"
#include "simulation/wingmate_orders.h"

namespace earthsiege {
struct tactical_destination {
  vec3<int32_t> committed{};
  vec3<int32_t> pending{};
  std::optional<size_t> committed_actor;
  std::optional<size_t> pending_actor;
  bool committed_active{false};
  bool pending_active{false};
  int32_t committed_kind{0};
  int32_t pending_kind{0};
};
struct tactical_control_state {
  int32_t command{-1};
  int32_t previous{-1};
  int32_t wingmate{-1};
  bool requires_destination{false};
  bool ready{false};
  bool cursor{false};
  bool blink{false};
  vec2<int32_t> pan{};
  vec3<int32_t> camera{0, 0, 200000};
  std::array<tactical_destination, 3> orders;
  wingmate_order packet;
};
std::optional<size_t> cycle_tactical_target(std::span<tactical_contact const> contacts, vec3<int32_t> const &camera, rectangle const &bounds, vec3<int32_t> const &player, uint8_t side, std::optional<size_t> previous, unsigned projection_shift = 7);

/// Original lower-map selection and order packet lifetime, independent of drawing.
class tactical_controller {
  tactical_control_state current;
  bool expansion{false};
  int32_t maximum_height{1000000};
  int32_t zoom_step{50000};
public:
  explicit tactical_controller(bool expansion = false);
  void initialize_height(int32_t height);
  tactical_control_state const &state() const;
  void camera(vec3<int32_t> const &position);
  void wingmate(int32_t slot);
  void choose(int32_t command);
  void pan(int x, int y);
  void recenter();
  void zoom(bool out);
  std::pair<vec3<int32_t>, std::optional<size_t>> pick(int x, int y, rectangle const &bounds, std::span<tactical_contact const> contacts) const;
  bool destination(vec3<int32_t> const &point, std::optional<size_t> actor, std::span<tactical_contact const> contacts, vec3<int32_t> const &player, int32_t range);
  void designate(size_t actor);
  void commit();
  wingmate_order const &packet(size_t issuer);
  void cancel();
};
} // namespace earthsiege
