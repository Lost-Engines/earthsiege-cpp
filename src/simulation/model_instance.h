#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "resources/model.h"

namespace earthsiege {

class model_instance {
  shape_part const &source;
  std::vector<uint16_t> cells;
  std::vector<animation_pose> local;
  std::vector<model_transform> world;
  std::vector<bool> changed;

public:
  model_instance(shape_part const &shape);
  void restore(model_instance const &saved);
  void reset_pose();
  void apply_frame(unsigned int sequence, unsigned int frame, unsigned int next_sequence, unsigned int next_frame, int16_t fraction);
  void update();
  std::span<animation_pose const> poses() const;
  std::span<model_transform const> transforms() const;
  std::span<uint16_t> cell_frames();
  std::span<uint16_t const> cell_frames() const;
};

} // namespace earthsiege
