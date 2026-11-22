#pragma once
#include <array>
#include <memory>
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "resources/presentation.h"
#include "resources/resource_store.h"

namespace earthsiege {

struct presentation_state {
  uint8_t page{0};
  int32_t mode{0};                                                             // 0 advance, 1 repeat current page, 2 stopped, 3 end label
  bool wait_latched{false};
  int16_t remaining{0};
  int16_t period{0};
  int16_t jump{-1};
  uint32_t random{0};
};

/// Native TTM scheduler; drawing commands are retained separately so repainting
/// a modern window does not advance timers or consume presentation randomness.
class presentation_player {
  resource_store const &resources;
  std::string volume;
  std::string directory;
  presentation_program program;
  presentation_state current;
  uint8_t selected_bank{0};
  using shape_bank = std::vector<bitmap>;
  std::array<std::shared_ptr<shape_bank const>, 6> banks;
  struct brush_command {
    std::shared_ptr<shape_bank const> shapes;
    size_t frame;
    int x;
    int y;
    unsigned int flip;
  };
  std::vector<brush_command> brushes;
  void execute();
public:
  presentation_player(resource_store const &resources, std::string volume, std::string const &path, uint32_t seed = 0);
  void advance(int16_t elapsed);
  void select_label(int16_t label);
  presentation_state const &state() const;
  void draw(framebuffer &target, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
