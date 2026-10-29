#pragma once
#include <array>
#include <memory>
#include <functional>
#include <SDL.h>
#include "simulation/joystick_input.h"
namespace earthsiege {
/// SDL is the electrical boundary. The original calibration and response run in core.
class joystick_devices {
 using handle=std::unique_ptr<SDL_Joystick,decltype(&SDL_JoystickClose)>;
 std::array<handle,2> sticks{{{nullptr,SDL_JoystickClose},{nullptr,SDL_JoystickClose}}};
 std::function<bool(int)> accept;
 void refresh();
public:
 explicit joystick_devices(std::function<bool(int)> filter={});
 ~joystick_devices();
 joystick_devices(joystick_devices const &)=delete;
 joystick_devices &operator=(joystick_devices const &)=delete;
 std::array<joystick_hardware,2> poll(uint8_t model,bool pedals);
 int16_t throttle()const;
};
}
