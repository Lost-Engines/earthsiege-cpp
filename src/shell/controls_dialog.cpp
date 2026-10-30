#include "controls_dialog.h"
#include <utility>
namespace earthsiege {
namespace {
std::string calibration_path(unsigned i){return "data/host-joy"+std::to_string(i+1)+".cal";}
}
std::array<joystick_calibration,2> load_host_calibrations(save_store const &saves){
 std::array<joystick_calibration,2> result;
 for(unsigned i{};i<2;++i){
  if(auto data{saves.read(calibration_path(i))})result[i]=read_joystick_calibration(*data);
  else{result[i].maximum.fill(65535);result[i].centre_low.fill(32767);result[i].centre_high.fill(32768);}
 }
 return result;
}
controls_dialog::controls_dialog(resource_store const &r,preferences &p,save_store const &s,std::array<joystick_calibration,2> &c,bool flight)
 :prefs{p},saves{s},calibrations{c},settings{p},artwork{r,flight},calibration_view{r,flight},backdrop{read_bitmaps(r.read("shell0","res32/regn1000.dbm")).front()},in_flight{flight}{focus_button(0);}
rectangle controls_dialog::button(unsigned i)const{return calibration?calibration_view.button(i):artwork.button(i);}
void controls_dialog::focus_button(int i){focus=i;auto b{button(static_cast<unsigned>(i))};warp=vec2<int32_t>{(b.left+b.right)/2,(b.top+b.bottom)/2};}
void controls_dialog::activate(unsigned i,bool backward,bool joystick){
 if(calibration){
  calibration->activate(i);
  if(calibration->done()){calibration.reset();focus_button(6);}
  return;
 }
 auto const result{settings.activate(i,backward,joystick)};
 if(result==control_setting_action::none)return;
 constexpr std::array<uint8_t,5> selected{7,8,9,10,11};
 auto const stored{prefs.save_selected(saves.read("data/prefs.cfg"),selected)};saves.write("data/prefs.cfg",stored);
 if(result==control_setting_action::done){done_=true;return;}
 auto const &v{prefs.current()};
 calibration.emplace(v[7],v[9]!=0,std::array<bool,2>{hardware[0].present,hardware[1].present},calibrations);focus_button(0);
}
void controls_dialog::sample(){
 if(calibration&&calibration->active()){
  auto const slot{calibration->selected_slot()};
  if(auto result{calibration->sample(hardware.at(slot).axes)}){
   saves.write(calibration_path(result->slot),write_joystick_calibration(result->calibration));focus_button(2);
  }
 }
}
void controls_dialog::key(uint16_t scan){
 if(calibration&&calibration->active()){if(scan==1)calibration->cancel();else if(scan)sample();return;}
 if(scan==15||(scan==82&&prefs.current()[10]))focus_button((focus+1)%static_cast<int>(count()));
 else if(scan==0x20f)focus_button((focus+static_cast<int>(count())-1)%static_cast<int>(count()));
 else if(scan==1)activate(count()-1);
 else if(scan==28)activate(static_cast<unsigned>(focus));
}
void controls_dialog::mouse_down(int x,int y,bool right){
 pressed=-1;reverse=right;
 if(calibration&&calibration->active())return;
 for(unsigned i{};i<count();++i)if(button(i).contains(x,y)){pressed=static_cast<int>(i);focus=pressed;break;}
}
void controls_dialog::mouse_up(int x,int y,bool right){
 auto const previous{std::exchange(pressed,-1)};
 if(previous>=0&&right==reverse&&button(static_cast<unsigned>(previous)).contains(x,y))activate(static_cast<unsigned>(previous),right);
}
void controls_dialog::devices(std::array<joystick_hardware,2> const &value){
 hardware=value;bool const fire{((value[0].buttons|value[1].buttons)&5)!=0};
 if(fire&&!fire_held){
  if(calibration&&calibration->active())sample();
  else activate(static_cast<unsigned>(focus),false,true);
 }
 fire_held=fire;
}
std::optional<vec2<int32_t>> controls_dialog::take_warp(){return std::exchange(warp,{});}
void controls_dialog::draw(framebuffer &f)const{
 if(!in_flight)f.image(backdrop,0,0);
 if(calibration){
  std::array<uint8_t,3> faces{};if(pressed>=0)faces.at(static_cast<size_t>(pressed))=1;
  calibration_view.draw(f,static_cast<unsigned>(calibration->choice()),calibration->message(),faces);
 }else{
  std::array<uint8_t,7> faces{};if(pressed>=0)faces.at(static_cast<size_t>(pressed))=1;
  artwork.draw(f,std::span<uint8_t const,5>{settings.state().data(),5},faces);
 }
}
}
