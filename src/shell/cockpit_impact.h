#pragma once
#include <array>
#include <cstdint>
#include "simulation/random_state.h"
namespace earthsiege {
struct cockpit_impact_view {
 unsigned mode{};
 bool transitioning{};
 bool palette_available{true};
 std::array<int32_t,2> origin{};
};
/// DBSIM's secondary-RNG impact flash and page-origin displacement.
/// The outside projection stays fixed; cockpit art, clips and widgets move.
class cockpit_impact {
public:
 struct state {
  uint32_t expiry{},next_flash{};
  bool flash{},impact_palette{},saved_palette{},alternate_palette{},shaking{};
  std::array<int32_t,2> saved_origin{},current_origin{};
  int32_t lower{},upper{};
  int16_t displacement{};
 };
private:
 state current;
 void start_shake(cockpit_impact_view const &,int amount=5);
 void move(unsigned amount,unsigned mode);
 void restore(unsigned mode);
public:
 void change_view(unsigned previous_mode,cockpit_impact_view const &);
 void hit(uint32_t now,cockpit_impact_view const &,random_state &);
 bool update(uint32_t now,cockpit_impact_view const &,random_state &);
 state const &snapshot() const { return current; }
};
}
