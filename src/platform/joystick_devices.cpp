#include "joystick_devices.h"
#include <stdexcept>
namespace earthsiege {
namespace {
int32_t axis(SDL_Joystick *s,int i){return s&&i<SDL_JoystickNumAxes(s)?int32_t(SDL_JoystickGetAxis(s,i))+32768:32768;}
uint8_t buttons(SDL_Joystick *s){
 uint8_t b{};if(s)for(int i{};i<4&&i<SDL_JoystickNumButtons(s);++i)if(SDL_JoystickGetButton(s,i))b|=static_cast<uint8_t>(1<<i);return b;
}
}
joystick_devices::joystick_devices(std::function<bool(int)> filter):accept{std::move(filter)}{
 if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)!=0)throw std::runtime_error{SDL_GetError()};
 SDL_JoystickEventState(SDL_ENABLE);
}
joystick_devices::~joystick_devices(){for(auto &s:sticks)s.reset();SDL_QuitSubSystem(SDL_INIT_JOYSTICK);}
void joystick_devices::refresh(){
 for(auto &s:sticks)if(s&&!SDL_JoystickGetAttached(s.get()))s.reset();
 for(int i{};i<SDL_NumJoysticks();++i){
  if(accept&&!accept(i))continue;
  auto const id{SDL_JoystickGetDeviceInstanceID(i)};bool opened{};
  for(auto const &s:sticks)if(s&&SDL_JoystickInstanceID(s.get())==id)opened=true;
  if(!opened)for(auto &s:sticks)if(!s){s.reset(SDL_JoystickOpen(i));break;}
 }
 SDL_JoystickUpdate();
}
std::array<joystick_hardware,2> joystick_devices::poll(uint8_t model,bool pedals){
 refresh();std::array<joystick_hardware,2> out;
 auto *a{sticks[0].get()},*b{sticks[1].get()};
 if(a){
  out[0].present=true;out[0].buttons=buttons(a);
  for(int i{};i<4;++i)out[0].axes[static_cast<size_t>(i)]=axis(a,i);
  int const slider{SDL_JoystickNumAxes(a)>3?3:2};
  uint8_t const hat{SDL_JoystickNumHats(a)?SDL_JoystickGetHat(a,0):uint8_t(SDL_HAT_CENTERED)};
  // Single cardinal position; diagonals select up, right, down, then left.
  int const direction{hat&SDL_HAT_UP?0:hat&SDL_HAT_RIGHT?1:hat&SDL_HAT_DOWN?2:hat&SDL_HAT_LEFT?3:-1};
  if(model==3)out[0].axes[3]=direction<0?32768:direction==0?0:direction==1?16384:direction==2?8192:24576;
  if(model==4||model==5){
   out[0].axes[3]=SDL_JoystickNumAxes(a)>slider?axis(a,slider)/2:16384;
   if(model==5&&direction>=0)out[0].buttons=static_cast<uint8_t>(direction==0?15:direction==1?7:direction==2?11:3);
  }
 }
 if(b){out[1].present=true;out[1].axes[2]=axis(b,0);out[1].axes[3]=axis(b,1);out[1].buttons=static_cast<uint8_t>((buttons(b)&3)<<2);}
 else if(a&&((model==2&&SDL_JoystickNumAxes(a)>=4)||(pedals&&SDL_JoystickNumAxes(a)>=3))){
  out[1].present=true;out[1].axes[2]=axis(a,2);out[1].axes[3]=axis(a,3);out[1].buttons=static_cast<uint8_t>(buttons(a)&12);
 }
 return out;
}
int16_t joystick_devices::throttle()const{
 auto *s{sticks[0].get()};if(!s||!SDL_JoystickGetAttached(s)||SDL_JoystickNumAxes(s)<3)return 0;
 int const i{SDL_JoystickNumAxes(s)>3?3:2};return static_cast<int16_t>(axis(s,i)/128-256);
}
}
