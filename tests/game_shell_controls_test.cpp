#include "shell/game_shell.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
struct cleanup{std::filesystem::path path;~cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}};
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};
 cleanup temp{std::filesystem::path{argv[2]}/("controls-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
 save_store saved{temp.path};game_shell shell{resources,temp.path,17};
 shell.action(1300);while(shell.transition_active())shell.advance(20);shell.action(1320);
 require(shell.screen()==1320,"controls reachable from options");
 control_settings_artwork controls{resources};calibration_artwork calibration{resources};
 auto click=[&](rectangle b,bool right=false){int x{(b.left+b.right)/2},y{(b.top+b.bottom)/2};shell.mouse_down(x,y,right);shell.mouse_up(x,y,right);};
 auto initial{shell.take_pointer_warp()};require(initial.has_value(),"keyboard focus warps to original button centre");
 click(controls.button(0),true);require(shell.settings().current()[7]==5,"right-click selects preceding device");
 click(controls.button(0));click(controls.button(0));require(shell.settings().current()[7]==1&&shell.settings().current()[8]==1,"standard stick restores remembered role");
 std::array<joystick_hardware,2> hardware;hardware[0].present=true;shell.devices(hardware);
 click(controls.button(5));require(shell.screen()==1320,"calibration stays in control modal");
 click(calibration.button(1));
 hardware[0].axes={1000,2000,32768,32768};shell.devices(hardware);shell.key(28);
 hardware[0].axes={60000,61000,32768,32768};shell.devices(hardware);shell.key(28);
 hardware[0].axes={30000,31000,32768,32768};shell.devices(hardware);shell.key(28);
 auto const &cal{shell.device_calibrations()[0]};
 require(cal.minimum[0]==1000&&cal.minimum[1]==2000&&cal.maximum[0]==60000&&cal.centre_low[1]==31000,"keyboard calibration uses latest live axes");
 auto file{saved.read("data/host-joy1.cal")};require(file.has_value()&&read_joystick_calibration(*file)==cal,"host calibration persisted in original format");
 click(calibration.button(2));shell.key(0x20f);auto focus{shell.take_pointer_warp()};auto b{controls.button(5)};
 require(focus&&b.contains(focus->x,focus->y),"Alt+Tab follows original controls focus order");
 shell.key(1);require(shell.screen()==1300,"Escape accepts controls and returns to options");
 auto prefs{saved.load_preferences()};require(prefs.current()[7]==1&&prefs.current()[8]==1,"controls saved");
 game_shell reopened{resources,temp.path,19};require(reopened.device_calibrations()[0]==cal,"calibration survives restart");
 reopened.action(1300);while(reopened.transition_active())reopened.advance(20);reopened.action(1320);
 framebuffer frame;reopened.draw(frame);frame.save(std::filesystem::path{argv[2]}/"controls-live.ppm",reopened.colours());
 std::cout<<"Original controls navigation, right-click, live calibration, persistence and reload pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
