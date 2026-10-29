#include "platform/joystick_devices.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
int main()try{
 SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
 joystick_devices devices{[](int i){return SDL_JoystickIsVirtual(i)==SDL_TRUE;}};
 int first{SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_FLIGHT_STICK,4,4,1)};
 require(first>=0,"virtual joystick attachment");auto *a{SDL_JoystickOpen(first)};
 auto set=[&](int x,int y,int z,int w){SDL_JoystickSetVirtualAxis(a,0,static_cast<int16_t>(x));SDL_JoystickSetVirtualAxis(a,1,static_cast<int16_t>(y));SDL_JoystickSetVirtualAxis(a,2,static_cast<int16_t>(z));SDL_JoystickSetVirtualAxis(a,3,static_cast<int16_t>(w));};
 set(-32768,32767,8192,-16384);SDL_JoystickSetVirtualButton(a,0,1);SDL_JoystickSetVirtualButton(a,2,1);
 auto p{devices.poll(2,false)};
 require(p[0].present&&p[1].present&&p[0].axes[0]==0&&p[0].axes[1]==65535&&p[1].axes[2]==40960&&p[1].axes[3]==16384,"four-axis fallback");
 require(p[0].buttons==5&&p[1].buttons==4,"fallback buttons");
 require(devices.throttle()==-128,"WCS host lever");
 SDL_JoystickSetVirtualHat(a,0,SDL_HAT_RIGHT);
 p=devices.poll(3,false);require(p[0].axes[3]==16384,"FCS resistor-ladder hat");
 p=devices.poll(5,false);require(p[0].buttons==7&&p[0].axes[3]==8192,"CH Pro hat and slider");
 int second{SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_FLIGHT_STICK,2,2,0)};
 require(second>=0,"second virtual joystick");auto *b{SDL_JoystickOpen(second)};
 SDL_JoystickSetVirtualAxis(b,0,-30000);SDL_JoystickSetVirtualAxis(b,1,30000);SDL_JoystickSetVirtualButton(b,1,1);
 p=devices.poll(2,false);require(p[1].axes[2]==2768&&p[1].axes[3]==62768&&p[1].buttons==8,"second stick owns second pair");
 SDL_JoystickDetachVirtual(second);SDL_JoystickClose(b);
 p=devices.poll(1,true);require(p[1].present&&p[1].axes[2]==40960,"unplug restores pedal axis fallback");
 SDL_JoystickDetachVirtual(first);SDL_JoystickClose(a);
 p=devices.poll(1,false);require(!p[0].present&&!p[1].present&&p[0].buttons==0&&devices.throttle()==0,"unplug clears physical input");
 std::cout<<"SDL virtual axes, hats, buttons, two-device selection and hot unplug pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
