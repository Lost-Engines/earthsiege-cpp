#pragma once
#include "render/framebuffer.h"
#include "resources/career.h"
#include "resources/resource_store.h"
namespace earthsiege {
struct mount_artwork {
  int16_t weapon{0};
  int16_t image{0};
  int16_t hit_x{0};
  int16_t hit_y{0};
  int16_t draw_x{0};
  int16_t draw_y{0};
};
/// Original shell CFG tables: compatibility, selection locations and equipment art.
class armament_catalog {
  std::vector<std::vector<std::vector<mount_artwork>>> chassis_;
  std::vector<std::vector<rectangle>> hotspots_;
public:
  explicit armament_catalog(resource_store const &resources);
  auto const &mounts(int16_t chassis) const {
    return chassis_.at(human_herc_index(chassis));
  }
  std::optional<int> hit(int16_t chassis, int x, int y) const;
  auto const &hotspots(int16_t chassis) const {
    return hotspots_.at(human_herc_index(chassis));
  }
  mount_artwork const *find(int16_t chassis, int mount, int16_t weapon) const;
  std::vector<int16_t> choices(player_herc const &herc, int mount,
    technology_state const &technology, armory const &inventory) const;
  /// GO 0x29abc. Exchange condition-preserving inventory entries immediately.
  bool exchange(player_herc &herc, int mount, int16_t weapon, armory &inventory) const;
};
/// Original next/previous hangar selection; single missions reassign the viewed pilot.
void cycle_arming_herc(hangar &roster, career_state &crew, int direction, bool single);
void select_missile_type(player_herc &herc, int mount, int16_t type);
} // namespace earthsiege
