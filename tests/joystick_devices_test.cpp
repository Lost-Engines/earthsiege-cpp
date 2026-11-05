#include "platform/joystick_devices.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
int main(int, char **)try{
 std::vector<host_joystick> host{{.id=0, .axes={-1,1,8192.0f/32767,-.5f}, .buttons={1,0,1,0}, .hats={0}}};
 joystick_devices devices{[&]{return host;}};
 auto p{devices.poll(2,false)};
 require(p[0].present&&p[1].present&&p[0].axes[0]==0&&p[0].axes[1]==65535&&p[1].axes[2]==40960&&p[1].axes[3]==16384,"four-axis fallback");
 require(p[0].buttons==5&&p[1].buttons==4,"fallback buttons");
 require(devices.throttle()==-128,"WCS host lever");
 host[0].hats[0]=2;
 p=devices.poll(3,false);require(p[0].axes[3]==16384,"FCS resistor-ladder hat");
 p=devices.poll(5,false);require(p[0].buttons==7&&p[0].axes[3]==8192,"CH Pro hat and slider");
 host.push_back({.id=1, .axes={-30000.0f/32768,30000.0f/32767}, .buttons={0,1}});
 p=devices.poll(2,false);require(p[1].axes[2]==2768&&p[1].axes[3]==62768&&p[1].buttons==8,"second stick owns second pair");
 host.pop_back();
 p=devices.poll(1,true);require(p[1].present&&p[1].axes[2]==40960,"unplug restores pedal axis fallback");
 host.clear();
 p=devices.poll(1,false);require(!p[0].present&&!p[1].present&&p[0].buttons==0&&devices.throttle()==0,"unplug clears physical input");
 std::cout<<"GLFW host axes, hats, buttons, two-device selection and hot unplug pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
