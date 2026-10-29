#pragma once
#include <optional>
#include "render/communications_display.h"
#include "render/mfd_instruments.h"

namespace earthsiege {

struct mfd_state {
  mfd_page page{mfd_page::status};
  size_t range_index{2};
  size_t choice{};
  std::optional<size_t> selected_actor;
  uint8_t pending_paint{2};
  std::array<bool, 6> refresh{};
  uint32_t deadline{};
  mfd_page previous_page{mfd_page::status};
  bool automatic_camera{};
  bool allow_camera_switch{true};
};
struct mfd_paint { mfd_page page; bool full; auto operator<=>(mfd_paint const &) const = default; };
enum class mfd_action { none, toggle_radar, cycle_target, transmit };

/// Own MFD page/range/selection and the original two-stage refresh scheduling.
class mfd_controller {
  mfd_state data;
public:
  explicit mfd_controller(size_t player, uint32_t ticks = 0);
  mfd_state const &state() const;
  void select(mfd_page page, size_t player, std::optional<size_t> target);
  void request_update();
  void repaint();
  void select_choice(size_t choice);
  mfd_action button(size_t index, size_t player, std::optional<size_t> target, std::span<communications_choice const, 6> choices);
  std::vector<mfd_paint> advance(uint32_t ticks, bool visible, bool external_camera, bool eo_selected, size_t player, std::optional<size_t> target);
};

} // namespace earthsiege
