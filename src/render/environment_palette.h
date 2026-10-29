#pragma once
#include "resources/artwork.h"
#include "resources/world.h"
namespace earthsiege {
/// Original view-dependent reflection ramp at VGA entries 64..79.
class environment_palette {
  using colour=std::array<uint8_t,3>;
  std::array<colour,16> endpoints;
  colour base{},impact_colour{};
  std::array<int16_t,2> indices;
  int16_t override_value{-1};
  bool ready{};
public:
  explicit environment_palette(world_environment const &);
  void override_band(int16_t value) { override_value=value; }
  int16_t pending_override() const { return override_value; }
  void update(palette &,uint16_t pitch,bool enabled=true,bool impact=false);
};
}
