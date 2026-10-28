#pragma once
#include "resources/cockpit.h"
#include "vectorstorm/vector/vector2.h"
#include "simulation/target_selection.h"

namespace earthsiege {

struct scanner_contact {
  combat_actor_kind kind;
  uint8_t side;
  vec2<int32_t> position;
  vec2<int32_t> last_position;
  bool active;
  bool detected;
  bool last_position_unavailable;
  bool selected;
};
struct scanner_display_state {
  vec2<int32_t> player_position;
  uint16_t heading;
  uint32_t ticks;
  int32_t range;
  bool radar_active;
  std::vector<scanner_contact> contacts;                                     // original world order, excluding the player
};
class scanner_display {
  bitmap background;
  std::array<font, 3> fonts;                                                // blue, green, red
  std::array<std::string, 2> activity_labels;
  std::array<std::string, 2> distance_labels;
public:
  explicit scanner_display(resource_store const &resources);
  void draw(framebuffer &target, rectangle const &bounds, scanner_display_state const &state) const;
};

} // namespace earthsiege
