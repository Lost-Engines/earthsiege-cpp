#pragma once
#include <cstdint>
#include "resources/model.h"

namespace earthsiege {

class model_instance;

using animation_index = int16_t;
using animation_time = int16_t;
using animation_rate = int16_t;
inline animation_index constexpr any_animation_frame{-1};
inline animation_rate constexpr animation_rate_one{256};
inline int constexpr animation_blend_one{1024};

enum class animation_channel_flags : uint16_t {
  none = 0,
  arrived = 1U << 0,
  transition = 1U << 1,
};

struct animation_channel_state {
  animation_index sequence{0};
  animation_index frame{0};
  animation_index next_sequence{0};
  animation_index next_frame{0};
  animation_index exit_frame{-1};
  animation_index destination_sequence{0};
  animation_index destination_frame{0};
  animation_time transition_duration{0};
  animation_index transition_pose{0};
  animation_index requested_sequence{-1};
  bool stop_on_arrival{false};
  animation_time elapsed{0};
  animation_time duration{0};
  bool has_motion{false};
  animation_pose motion;
  model_transform accumulated;
  animation_rate rate{0};
  animation_channel_flags flags{animation_channel_flags::arrived};
};

class animation_channel {
  animation_list const &source;
  animation_channel_state current;

public:
  animation_channel(animation_list const &animation);
  void restore(animation_channel const &saved);
  void reset(animation_index sequence, animation_index frame = 0, animation_time elapsed = 0);
  void request_sequence(animation_index sequence, bool stop_on_arrival = false);
  void cancel_request();
  void rebase_motion();
  void set_rate(animation_rate rate);
  void advance(animation_time delta);
  void set_phase(animation_index sequence, int16_t phase);
  animation_channel_state const &state() const;
  int16_t fraction() const;
  model_transform root_motion() const;
  void apply(model_instance &instance) const;

private:
  animation_index next(animation_index sequence, animation_index frame) const;
  animation_index previous(animation_index sequence, animation_index frame) const;
  void load_motion(animation_index pose);
  void frame_motion();
  void accumulate(bool reverse);
  bool reached();
  bool has_flag(animation_channel_flags flag) const;
  void set_flag(animation_channel_flags flag, bool value);
};

} // namespace earthsiege
