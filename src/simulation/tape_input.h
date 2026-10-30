#pragma once
#include "resources/input_tape.h"
#include "joystick_routing.h"
namespace earthsiege {
/// Original 0x4cb14 playback path. Recorded axes are already routed; only the
/// post-read mode switch, camera suppression, WCS and backturn policy repeat.
/// Mouse events belong to the interface queue, not the direct pointer fields.
device_input_sample route_tape_frame(device_routing_options &,tape_frame const &,
                                    bool suppress_camera=false,int16_t throttle=0);
}
