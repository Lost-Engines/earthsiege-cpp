#include "mission_alert.h"
#include <stdexcept>
#include <utility>
namespace earthsiege {
mission_alert::mission_alert(resource_store const &r,int message,int initial_focus)
 :background{read_bitmaps(r.read("simvol0",message<2?"dba/gnl2alrt.dba":"dba/gnl_alrt.dba")).front()},
 corners{read_bitmaps(r.read("simvol0","dba/corners.dba"))},styles{read_button_styles(r.read("simvol0","dat/styles.dat"))},
 title{read_font(r.read("simvol0","dfn/title.dfn"))},body{read_font(r.read("simvol0","dfn/green6x8.dfn"))},button_metrics{read_font(r.read("simvol0","dfn/cpylw.dfn"))},
 button_fonts{read_font(r.read("simvol0","dfn/active.dfn")),read_font(r.read("simvol0","dfn/pushed.dfn")),read_font(r.read("simvol0","dfn/inactive.dfn"))},
 strings{read_interface_strings(r.read("simvol0","str/gnl_alrt.str"))},message_{message}{
  if(message<0||message>=19)throw std::out_of_range{"mission alert"};
  focus(initial_focus);
}
int mission_alert::count()const{switch(message_){case 0:case 2:case 3:case 7:case 8:case 17:case 18:return 1;default:return 2;}}
bool mission_alert::exits_mission(int response)const{
  // DBSIM 0x97158, compared by the original Q handler at 0x59fb7.
  constexpr std::array<int,19> exit_response{1,1,0,0,1,1,1,1,0,1,1,1,1,1,1,1,1,0,0};
  return message_>1 && response==exit_response.at(static_cast<size_t>(message_));
}
rectangle mission_alert::button(int i)const{int const left{count()==1?71:i==0?22:120};return {48+left,115,48+left+80,135};}
void mission_alert::draw(framebuffer &frame)const{
  int constexpr x{48},y{45};frame.image(background,x,y,true);
  auto const &caption{strings.at(0).at(static_cast<size_t>(message_)).text};int const width{title.text_width(caption)};
  rectangle const b{x+((222-width-20)>>1),y+4,x+((222-width-20)>>1)+width+20,y+21};
  frame.line(b.left,b.top,b.right,b.top,16);frame.line(b.left,b.top,b.left,b.bottom,16);frame.line(b.right,b.top,b.right,b.bottom,27);frame.line(b.left,b.bottom,b.right,b.bottom,28);
  frame.image(corners.at(0),b.left,b.top,true);frame.image(corners.at(1),b.right-4,b.top,true);frame.image(corners.at(2),b.left,b.bottom-4,true);frame.image(corners.at(3),b.right-4,b.bottom-4,true);
  frame.text(title,caption,b.left+((b.right-b.left)>>1)-((width-1)>>1),b.top+((17+static_cast<int>(title.height))>>1)-11);
  int lines{};while(lines<4&&!strings.at(2).at(static_cast<size_t>(message_*4+lines)).text.empty())++lines;
  for(int i{};i<lines;++i){auto const &text{strings.at(2).at(static_cast<size_t>(message_*4+i)).text};int const row{i+(lines==1?1:0)};
    frame.text(body,text,x+111-((body.text_width(text)-1)>>1),y+30+10*row+((10+static_cast<int>(body.height))>>1));}
  for(int i{};i<count();++i){auto const bounds{button(i)};int const state{pressed_==i?1:0};draw_dialog_button(frame,bounds,styles[static_cast<size_t>(state)],corners);
    auto const &text{strings.at(1).at(static_cast<size_t>(message_*2+i)).text};auto const &face{button_fonts[static_cast<size_t>(state)]};
    frame.text(face,text,bounds.left+40-((face.text_width(text)-1)>>1),bounds.top+((20+static_cast<int>(button_metrics.height))>>1)-3);
  }
}
void mission_alert::focus(int i){
 if(i<-1||i>=count())throw std::out_of_range{"alert focus"};
 focus_=i;if(i>=0){auto b{button(i)};warp=vec2<int32_t>{(b.left+b.right)/2,(b.top+b.bottom)/2};}
}
alert_input_result mission_alert::input(uint16_t scan,bool fire,bool next,bool wcs){
 if(fire){if(focus_<0){focus(0);return {true,{}};}return {true,focus_};}
 if(next){focus((focus_+1)%count());return {true,{}};}
 if(scan==1){focus(0);return {true,0};}
 if(scan==28){if(focus_<0){focus(0);return {true,{}};}return {true,focus_};}
 if(scan==15||(scan==82&&wcs))focus((focus_+1)%count());
 else if(scan==0x20f)focus(focus_<0?count()-1:(focus_+count()-1)%count());
 else if(scan!=82)return {};
 return {true,{}};
}
std::optional<int> mission_alert::devices(std::array<joystick_hardware,2> const &raw,bool wcs){
 auto bits{raw[0].buttons|raw[1].buttons};bool fire{(bits&5)!=0},next{(bits&10)!=0};
 auto result{input(0,fire&&!fire_held,next&&!next_held,wcs)};fire_held=fire;next_held=next;return result.response;
}
std::optional<vec2<int32_t>> mission_alert::take_warp(){return std::exchange(warp,{});}
void mission_alert::mouse_down(int x,int y){pressed_=-1;for(int i{};i<count();++i)if(button(i).contains(x,y)){pressed_=focus_=i;}}
std::optional<int> mission_alert::mouse_up(int x,int y){int const previous{pressed_};pressed_=-1;if(previous>=0&&button(previous).contains(x,y))return previous;return {};}
}
