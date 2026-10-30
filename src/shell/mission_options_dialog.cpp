#include "mission_options_dialog.h"
#include <utility>
namespace earthsiege {
mission_options_dialog::mission_options_dialog(resource_store const &r,preferences &p,save_store const &s,std::array<joystick_calibration,2> &c)
 :resources{r},prefs{p},saves{s},calibrations{c},settings{p},artwork{r}{focus_button(0);}
void mission_options_dialog::focus_button(int i){focus=i;auto b{artwork.button(static_cast<unsigned>(i))};warp=vec2<int32_t>{(b.left+b.right)/2,(b.top+b.bottom)/2};}
void mission_options_dialog::activate(unsigned i,bool backward){
 if(click)click();
 auto action{settings.activate(i,backward)};
 if(action==mission_preference_action::controls){controls=std::make_unique<controls_dialog>(resources,prefs,saves,calibrations,true);controls->feedback(click);return;}
 if(action==mission_preference_action::done){
  constexpr std::array<uint8_t,7> selected{0,1,2,3,4,5,6};bool changed{};
  for(auto index:selected)changed|=prefs.current()[index]!=prefs.saved()[index];
  if(changed)saves.write("data/prefs.cfg",prefs.save_selected(saves.read("data/prefs.cfg"),selected));
  done_=true;
 }
}
void mission_options_dialog::finish_child(){if(controls&&controls->done()){controls.reset();focus_button(7);}}
void mission_options_dialog::key(uint16_t scan){
 if(controls){controls->key(scan);finish_child();return;}
 if(scan==15||(scan==82&&prefs.current()[10]))focus_button((focus+1)%9);
 else if(scan==0x20f)focus_button((focus+8)%9);
 else if(scan==1)activate(8);
 else if(scan==28)activate(static_cast<unsigned>(focus));
 else if((scan==13||scan==78||scan==12||scan==74)&&focus>=4&&focus<=6)activate(static_cast<unsigned>(focus),scan==12||scan==74);
}
void mission_options_dialog::mouse_down(int x,int y,bool right){
 if(controls){controls->mouse_down(x,y,right);return;}
 pressed=-1;reverse=right;
 for(unsigned i{};i<9;++i)if(artwork.button(i).contains(x,y)){pressed=static_cast<int>(i);focus=pressed;break;}
}
void mission_options_dialog::mouse_up(int x,int y,bool right){
 if(controls){controls->mouse_up(x,y,right);finish_child();return;}
 auto const previous{std::exchange(pressed,-1)};
 if(previous>=0&&right==reverse&&artwork.button(static_cast<unsigned>(previous)).contains(x,y))activate(static_cast<unsigned>(previous),right);
}
void mission_options_dialog::devices(std::array<joystick_hardware,2> const &raw){
 if(controls){controls->devices(raw);finish_child();return;}
 auto const buttons{raw[0].buttons|raw[1].buttons};bool const fire{(buttons&5)!=0},next{(buttons&10)!=0};
 if(fire&&!fire_held)activate(static_cast<unsigned>(focus));
 else if(next&&!next_held)focus_button((focus+1)%9);
 fire_held=fire;next_held=next;
}
void mission_options_dialog::cancel_press(){pressed=-1;if(controls)controls->cancel_press();}
std::optional<vec2<int32_t>> mission_options_dialog::take_warp(){return controls?controls->take_warp():std::exchange(warp,{});}
void mission_options_dialog::draw(framebuffer &frame)const{
 std::array<uint8_t,9> faces{};if(pressed>=0)faces[static_cast<size_t>(pressed)]=1;
 artwork.draw(frame,std::span<uint8_t const,7>{prefs.current().data(),7},faces);
 if(controls)controls->draw(frame);
}
}
