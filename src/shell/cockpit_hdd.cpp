#include "cockpit_hdd.h"
#include <algorithm>
#include <utility>

namespace earthsiege {
namespace {
rectangle translate(rectangle b,int x,int y){b.left+=x;b.right+=x;b.top+=y;b.bottom+=y;return b;}
void label(framebuffer &target,font const &face,std::string_view text,rectangle const &b,uint8_t background,bool centred=true){
  target.fill(b,background);int const x{centred?b.left+((b.right-b.left)>>1)-((face.text_width(text)-1)>>1):b.left};
  target.text(face,text,x,b.top+((b.bottom-b.top+static_cast<int>(face.height))>>1));
}
void outline(framebuffer &target,rectangle const &b,uint8_t colour){target.fill({b.left,b.top,b.right,b.top},colour);target.fill({b.left,b.bottom,b.right,b.bottom},colour);target.fill({b.left,b.top,b.left,b.bottom},colour);target.fill({b.right,b.top,b.right,b.bottom},colour);}
}
cockpit_hdd::cockpit_hdd(resource_store const &r,cockpit_layout const &layout,mission_session &session,pointer_controls &input,random_state &rng)
 :random{rng},map{r,cache,session.scenery()},damage{r},map_state{session.tactical_state(0)},wingmates{session.wingmate_ids()},
 button_art{read_bitmaps(r.read("simvol0","dba/PBUTTONS.DBA"))},
 fonts{read_font(r.read("simvol0","dfn/CPGREEN.DFN")),read_font(r.read("simvol0","dfn/CPYLW.DFN")),read_font(r.read("simvol0","dfn/CPRED.DFN")),read_font(r.read("simvol0","dfn/CPBLUE.DFN"))},
 strings{read_interface_strings(r.read("simvol0","str/STRINGS0.STR"))}{
  auto const pilots{read_interface_strings(r.read("simvol0","str/PILOTS.STR")).at(0)};
  for(size_t i{};i<wingmates.size();++i)pilot_names[i]=pilots.at(static_cast<size_t>(session.pilot_identity(wingmates[i]).value()/3)).text;
  auto const parent{layout.bounds(0x4bc)};auto const rect{[&](size_t offset){return translate(layout.bounds(offset),parent.left,parent.top);}};
  bounds=rect(0x4cc);command_bounds=rect(0x4dc);list_bounds=rect(0x4ec);map_bounds=bounds;map_bounds.right=command_bounds.left-2;
  direction_art=layout.words[0x62c/4];for(size_t i{};i<buttons.size();++i){buttons[i]=rect(0x50c+i*16);button_ids[i]=input.add_button(buttons[i]);}
  int const step{(command_bounds.bottom-command_bounds.top)/9};
  for(size_t i{};i<rows.size();++i){auto b{command_bounds};b.top+=static_cast<int>(i+1)*step;b.bottom=b.top+static_cast<int>(fonts[0].height);rows[i]=b;row_ids[i]=input.add_button(b);}
  map_id=input.add_button(map_bounds);auto const &p{session.player().movement().position};controller.camera({p.x,p.y,initial_tactical_height(map_state.mission_bounds,map_bounds)});enable(input);
}
void cockpit_hdd::enable(pointer_controls &input){
  constexpr std::array<std::array<bool,15>,2> enabled{{{1,1,1,1,1,1,1,1,0,0,1,1,1,1,1},{1,1,1,1,1,1,0,0,0,0,1,1,1,0,0}}};
  auto const set{[&](size_t id,bool value){if((input.at(id).face!=control_face::disabled)!=value)input.enable(id,value);}};
  for(size_t i{};i<buttons.size();++i)set(button_ids[i],enabled[static_cast<size_t>(page)][i]);
  for(auto id:row_ids)set(id,page==0);
  set(map_id,page==0);
}
void cockpit_hdd::select_wingmate(int const slot,mission_session &session){
  if(slot<0 || static_cast<size_t>(slot)>=wingmates.size() || session.order_state(wingmates[static_cast<size_t>(slot)]).dead)return;
  if(controller.state().wingmate==slot)return;
  controller.wingmate(slot);session.request_wingmate_status(wingmates[static_cast<size_t>(slot)]);selected_actor.reset();prompt.reset();dirty=true;
}
void cockpit_hdd::choose(int const command,bool const keyboard){
  if(controller.state().wingmate<0){prompt=0;prompt_refreshes=6;dirty=true;return;}
  if(controller.state().command!=command){controller.choose(command);prompt=controller.state().requires_destination?std::optional<size_t>{command==11?3U:2U}:std::nullopt;dirty=true;}
  if(keyboard){mouse_x=map_bounds.left+((map_bounds.right-map_bounds.left)>>1);mouse_y=map_bounds.top+((map_bounds.bottom-map_bounds.top)>>1);warp=vec2<int32_t>{mouse_x-origin_x,mouse_y-origin_y};}
}
void cockpit_hdd::transmit(mission_session &session){
  auto const &s{controller.state()};if(s.wingmate<0){prompt=0;prompt_refreshes=6;dirty=true;return;}
  if(!s.ready){prompt=s.command<0?1U:s.command==11?3U:2U;prompt_refreshes=6;dirty=true;return;}
  auto const actor{wingmates.at(static_cast<size_t>(s.wingmate))};controller.commit();session.transmit_order(controller.packet(session.player_id()),actor);controller.cancel();selected_actor.reset();prompt.reset();dirty=true;
}
void cockpit_hdd::select_subject(int const direction,mission_session const &session){
  do{subject=(subject+direction+5)%5;}while(subject!=0 && subject!=4 && static_cast<size_t>(subject)>wingmates.size());
  static_cast<void>(session);scroll=0;dirty=true;
}
void cockpit_hdd::pick(mission_session &session,int32_t const range){
  auto const [p,actor]{controller.pick(mouse_x,mouse_y,map_bounds,map_state.contacts)};
  if(controller.state().requires_destination && controller.state().wingmate>=0){
    if(controller.destination(p,actor,map_state.contacts,session.player().movement().position,range)){sounds.push_back(20);prompt.reset();}
  }else if(actor){auto const it{std::ranges::find(wingmates,*actor)};if(it!=wingmates.end())select_wingmate(static_cast<int>(it-wingmates.begin()),session);}
  dirty=true;
}
void cockpit_hdd::pointer(int const x,int const y){mouse_x=x;mouse_y=y;}
void cockpit_hdd::execute_button(size_t i,mission_session &session,pointer_controls &input){
    if(i<2){page=static_cast<int>(i);dirty=true;enable(input);return;}
    switch(i){
      case 2:if(page==0)controller.pan(0,1);else{mode=(mode+1)%3;scroll=0;}break;
      case 3:if(page==0)controller.pan(0,-1);else{mode=(mode+2)%3;scroll=0;}break;
      case 4:if(page==0)controller.pan(-1,0);else select_subject(-1,session);break;
      case 5:if(page==0)controller.pan(1,0);else select_subject(1,session);break;
      case 6:if(page==0)controller.zoom(true);break;
      case 7:if(page==0)controller.zoom(false);break;
      case 10:case 11:case 12:page=0;enable(input);select_wingmate(static_cast<int>(i-10),session);break;
      case 13:if(page==0)transmit(session);break;
      case 14:controller.cancel();selected_actor.reset();prompt.reset();break;
      default:break;
    }dirty=true;
}
bool cockpit_hdd::activate(control_activation const &action,mission_session &session,pointer_controls &input,int32_t const range){
  for(size_t i{};i<button_ids.size();++i)if(action.index==button_ids[i]){
    // 3d430 stores only the last request; 3cd28 dispatches it after simulation.
    if(action.value&1)pending_button=i;
    return true;
  }
  if(page!=0)return false;
  if(action.index==map_id){if(!failure.failed)pick(session,range);return true;}
  for(size_t i{};i<row_ids.size();++i)if(action.index==row_ids[i]){if(!failure.failed){int const cmd{static_cast<int>(i)+10};if(action.double_click && controller.state().command==cmd && controller.state().ready)pending_button=13;else choose(cmd,false);}return true;}
  return false;
}
bool cockpit_hdd::command(uint16_t const code,mission_session &session,pointer_controls &input,int32_t const range){
  if(code==65 || code==66){page=code-65;dirty=true;enable(input);return true;}
  if(!visible)return false;
  auto const button{[&](size_t i){return activate({button_ids[i],1},session,input,range);}};
  switch(code){case 72:return button(2);case 80:return button(3);case 75:return button(4);case 77:return button(5);
    case 2:case 3:case 4:return button(code+8);case 13:case 78:return button(7);case 12:case 74:return button(6);
    case 31:case 23:case 17:page=1;mode=code==31?0:code==23?1:2;scroll=0;dirty=true;enable(input);return true;default:break;}
  if(page==1)return false;
  if(failure.failed)return false;
  constexpr std::array<uint16_t,8> keys{32,30,33,20,34,24,46,18};auto const it{std::ranges::find(keys,code)};
  if(it!=keys.end()){choose(10+static_cast<int>(it-keys.begin()),true);return true;}
  switch(code){
    case 45:return button(13);
    case 14:return button(14);
    case 52:if(controller.state().command>=10)choose((controller.state().command-9)%8+10,false);return true;
    case 51:if(controller.state().command>=10)choose((controller.state().command-3)%8+10,false);return true;
    case 76:controller.recenter();dirty=true;return true;
    case 28:{
      auto const &s{controller.state()};if(!s.requires_destination)return true;
      int const saved_x{mouse_x},saved_y{mouse_y};
      if(s.command==11 || s.command==12){
        if(selected_actor){for(auto const &c:map_state.contacts)if(c.actor==selected_actor){auto p{tactical_project(c.position,s.camera,map_bounds)};mouse_x=p.x;mouse_y=p.y;break;}}
        else if(s.command==11)return true;
      }
      pick(session,range);mouse_x=saved_x;mouse_y=saved_y;return true;
    }
    case 15:{
      auto const &s{controller.state()};if(s.wingmate<0 || (s.command!=11 && s.command!=12))return true;
      selected_actor=cycle_tactical_target(map_state.contacts,s.camera,map_bounds,session.player().movement().position,s.command==11?1:0,selected_actor);
      if(selected_actor){controller.designate(*selected_actor);sounds.push_back(26);prompt.reset();}
      else sounds.push_back(27);
      dirty=true;return true;
    }
    default:return false;
  }
}
void cockpit_hdd::paint(mission_session &session){
  cache.fill(painted,63);
  if(page==1){auto selected{subject==0?std::optional<size_t>{session.player_id()}:subject==4?session.target_selection():std::optional<size_t>{wingmates.at(static_cast<size_t>(subject-1))}};
    auto s{session.full_damage(selected)};s.mode=mode;s.scroll=scroll;s.subject=subject;s.failed=failure.failed;
    if(subject>0 && subject<4)s.subject_name=pilot_names.at(static_cast<size_t>(subject-1));
    if(mode==2 && s.model>=0){
      // Original global text buffers are packed, but empty slot pointers are
      // retained. Switching HERCs can therefore expose an old slot label.
      size_t next{};
      for(size_t i{};i<s.weapons.size();++i)if(!s.weapons[i].name.empty()){
        weapon_label_refs.at(i)=next;weapon_label_pool.at(next++)=s.weapons[i].name;
      }
      for(auto const ref:weapon_label_refs)s.weapon_labels.push_back(ref?weapon_label_pool.at(*ref):std::string{});
    }
    damage.draw(cache,painted,translate(list_bounds,-origin_x,-origin_y),s);return;}
  auto const &s{controller.state()};map_state.camera=s.camera;map_state.failed=failure.failed;
  for(auto &c:map_state.contacts){c.selected=c.actor && c.actor==selected_actor;c.destination.reset();c.blink=false;
    if(s.wingmate>=0 && c.actor==wingmates.at(static_cast<size_t>(s.wingmate))){
      auto const &o{s.orders[static_cast<size_t>(s.wingmate)]};c.blink=s.blink;
      if(o.pending_active || o.committed_active){bool const pending{o.pending_active};auto const kind{pending?o.pending_kind:o.committed_kind};auto const actor{pending?o.pending_actor:o.committed_actor};
        if(kind==0)c.destination=pending?o.pending:o.committed;
        else for(auto const &other:map_state.contacts)if(other.actor && other.actor==actor){c.destination=other.position;break;}
        c.destination_colour=static_cast<uint8_t>(std::array{56,59,49}[static_cast<size_t>(s.wingmate)]-(pending?0:1));
      }
    }
  }
  map.draw(translate(map_bounds,-origin_x,-origin_y),map_state);
  for(size_t i{};i<rows.size();++i){auto const b{translate(rows[i],-origin_x,-origin_y)};auto const &entry{strings.at(0).at(i+10)};bool const disabled{s.wingmate<0 || failure.failed};auto const &face{fonts[disabled?3:s.command==static_cast<int>(i)+10?1:0]};
    label(cache,face,entry.text,{b.left+5,b.top,b.right,b.bottom},63,false);
    if(!entry.attributes.empty() && entry.attributes[0]<entry.text.size()){auto const k{entry.attributes[0]};int const x{b.left+5+face.text_width(std::string_view{entry.text}.substr(0,k))};cache.text(fonts[disabled?3:2],std::string_view{entry.text}.substr(k,1),x,b.top+((b.bottom-b.top+static_cast<int>(face.height))>>1));}
    if(s.command==static_cast<int>(i)+10)cache.fill({b.left+1,b.top+3,b.left+3,b.bottom},59);
  }
  if(prompt){auto b{translate(command_bounds,-origin_x,-origin_y)};b.bottom=b.top+static_cast<int>(fonts[0].height);label(cache,fonts[2],strings.at(28).at(*prompt).text,b,58);}
}
void cockpit_hdd::update(mission_session &session,pointer_controls &input,camera_state const &camera,uint32_t const ticks,int const x,int const y){
  if(pending_button && (!failure.failed || *pending_button<=1)){
    auto const button{*pending_button};pending_button.reset();execute_button(button,session,input);
  }
  auto const b{translate(bounds,-x,-y)};bool const shown{camera.mode==camera_mode::attached && b.right>=0 && b.left<320 && b.bottom>=0 && b.top<200};
  if(shown && (!visible || x!=origin_x || y!=origin_y))dirty=true;
  visible=shown;origin_x=x;origin_y=y;painted=b;
  if(camera.mode==camera_mode::attached){bool const was{failure.failed};if(advance_instrument_failure(failure,instrument_damage_band(session.player_actor().internal_damage_fraction(4)),ticks,random))sounds.push_back(28);if(was!=failure.failed)dirty=true;}
  if(!visible)return;
  if(ticks>deadline){deadline=ticks+30;blink_phase=!blink_phase;if(prompt_refreshes && --prompt_refreshes==0)prompt.reset();dirty=true;}
  map_state=session.tactical_state(ticks);map_state.blink_phase=blink_phase;auto const &p{session.player().movement().position};auto const &s{controller.state()};controller.camera({p.x+s.pan.x,p.y+s.pan.y,s.camera.z});
  if(dirty){paint(session);dirty=false;}enable(input);
}
void cockpit_hdd::draw(framebuffer &frame,pointer_controls const &input,int const x,int const y)const{
  if(!visible)return;
  for(int py{std::max(0,painted.top)};py<=std::min(199,painted.bottom);++py)for(int px{std::max(0,painted.left)};px<=std::min(319,painted.right);++px)frame.pixel(px,py,cache.pixels()[static_cast<size_t>(py*320+px)]);
  for(size_t i{};i<buttons.size();++i){auto const b{translate(buttons[i],x,y)};bool const pressed{input.at(button_ids[i]).face!=control_face::normal};
    if(i<8){auto const base{i<2?22+2*static_cast<int>(i):i<6?direction_art+2*static_cast<int>(i-2):51+2*static_cast<int>(i-6)};frame.image(button_art.at(static_cast<size_t>(base)+(pressed?1:0)),b.left,b.top,true);}
    else if(i==9)label(frame,fonts[2],page==0?strings.at(10).at(0).text:strings.at(11).at(static_cast<size_t>(mode)).text,b,63);
    else if(i>=10 && i<=12)outline(frame,b,controller.state().wingmate==static_cast<int>(i-10)?56:63);
    else if(i>=13 && page==0)label(frame,fonts[2],strings.at(8).at(i-13).text,b,pressed && (i==14 || controller.state().ready)?59:63);
  }
}
std::vector<uint16_t> cockpit_hdd::take_sounds(){return std::exchange(sounds,{});}
std::optional<vec2<int32_t>> cockpit_hdd::take_warp(){return std::exchange(warp,{});}
tactical_control_state const &cockpit_hdd::state()const{return controller.state();}
} // namespace earthsiege
