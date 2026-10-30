#pragma once
#include "render/projection.h"
#include "resources/artwork.h"
#include "simulation/random_state.h"
#include <optional>
namespace earthsiege {
/// Original DBSIM 57ffc lift and landing shake; world simulation is suspended.
/// Time is the external 60 Hz clock, while lift motion is per rendered frame.
class cockpit_startup {
  palette source_,colours_;
  std::array<std::array<int16_t,3>,256> increments_;
  unsigned lift_{};
  int16_t displacement_{};
  uint32_t shake_expiry_{};
  std::optional<uint32_t> fade_expiry_;
  bool fading_{true},finished_{};
public:
  explicit cockpit_startup(palette const &target);
  bool lifting()const{return lift_<115;}
  bool finished()const{return finished_;}
  int32_t height_offset()const{return lifting()?-4000+static_cast<int32_t>(lift_)*35:-10;}
  int16_t displacement()const{return displacement_;}
  int cover_y(int32_t camera_height,camera_projection const &)const;
  void prepare_frame(random_state &);
  void update_palette(uint32_t now,int cover_y);
  void presented(uint32_t now);
  void drawn(uint32_t now);
  palette const &colours()const{return colours_;}
};
} // namespace earthsiege
