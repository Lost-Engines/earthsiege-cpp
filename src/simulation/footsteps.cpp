#include "footsteps.h"
#include "simulation/fixed_math.h"

namespace earthsiege {
void place_ground_attachment(herc_ground_attachment &attachment,
  vec3<int32_t> const &local, model_transform const &actor, uint16_t const heading) {
  attachment.position=rotate_vector(local,actor);
  for(unsigned axis{};axis<3;++axis)attachment.position[axis]=wrap_long(static_cast<int64_t>(attachment.position[axis])+actor.translation[axis]);
  attachment.heading=heading;
}

std::vector<uint16_t> advance_footsteps(footstep_state &state, movement_profile const &p,
  std::span<int16_t const,10> trigger, std::span<int16_t const,10> rearm,
  animation_channel_state const &channel, int16_t const rate, bool const player,
  std::span<footstep_attachment const> attachments) {
  std::vector<uint16_t> result;
  if(!player) return result;
  auto const sequence{channel.requested_sequence == -1 ? channel.sequence : channel.requested_sequence};
  size_t latch{};
  for(auto const &attachment : attachments) {
    if(!attachment.active) continue; // Original advances its latch pointer only for live attachments.
    int pair{-1}; uint16_t sound{}; bool forward{true}; int32_t value{};
    if(sequence == p.disabled_sequence) {
      if(attachment.type == 2) { latch=4; pair=3; value=attachment.world_z; sound=31; }
    } else if(attachment.type == 0) {
      if(sequence == p.movement_sequence || sequence == p.turn_sequence || sequence == p.forward_stop_sequence || sequence == p.reverse_stop_sequence) {
        forward=rate>0; pair=forward ? 0 : 1;
      } else if(sequence == p.fast_sequence) pair=2;
      if(pair>=0) {
        value=attachment.local_y; sound=29;
        if(forward ? value>rearm[static_cast<size_t>(pair)] : value<rearm[static_cast<size_t>(pair)]) state.latches.at(latch)=1;
      }
    }
    if(pair>=0 && state.latches.at(latch) && (forward ? value<trigger[static_cast<size_t>(pair)] : value>trigger[static_cast<size_t>(pair)])) {
      result.push_back(sound); state.latches[latch]=0;
    }
    ++latch;
  }
  return result;
}
} // namespace earthsiege
