#include "tactical_controller.h"
#include <stdexcept>

namespace earthsiege {
std::optional<size_t> cycle_tactical_target(std::span<tactical_contact const> contacts,vec3<int32_t> const &camera,rectangle const &bounds,vec3<int32_t> const &player,uint8_t const side,std::optional<size_t> previous){
  size_t start{};
  if(previous)for(size_t i{};i<contacts.size();++i)if(contacts[i].actor==previous && contacts[i].side==side){start=i+1;break;}
  for(size_t j{};j<contacts.size();++j){
    auto const &c{contacts[(start+j)%contacts.size()]};
    if(!c.actor || c.side!=side || c.dead || !c.selectable)continue;
    auto const p{tactical_project(c.position,camera,bounds)};if(!bounds.contains(p.x,p.y))continue;
    if(side && !c.known && !(c.detected && fast_distance({c.position.x-player.x,c.position.y-player.y,c.position.z-player.z})<1000000))continue;
    return c.actor;
  }
  return {};
}

tactical_control_state const &tactical_controller::state()const{return current;}
void tactical_controller::camera(vec3<int32_t> const &position){current.camera=position;}
void tactical_controller::wingmate(int32_t const slot){
  if(slot < -1 || slot>=3)throw std::out_of_range{"tactical wingmate slot"};
  if(current.wingmate!=-1 && current.command!=-1){
    choose(-1);current.orders[static_cast<size_t>(current.wingmate)].pending_active=false;
    current.ready=false;current.blink=false;current.requires_destination=false;
  }
  current.wingmate=slot;if(slot!=-1)current.blink=true;
}
void tactical_controller::choose(int32_t const command){
  current.requires_destination=command>=11 && command<=14;
  current.cursor=current.requires_destination;current.ready=!current.requires_destination;
  if(current.command!=command){current.previous=current.command;current.command=command;}
}
void tactical_controller::pan(int const x,int const y){
  auto const step{tactical_pan_step(current.camera.z)};
  if(current.camera.x+x*step>=-5000000 && current.camera.x+x*step<=5000000)current.pan.x+=x*step;
  if(current.camera.y+y*step>=-5000000 && current.camera.y+y*step<=5000000)current.pan.y+=y*step;
}
void tactical_controller::recenter(){current.pan={0,0};}
void tactical_controller::zoom(bool const out){
  auto const next{current.camera.z+(out?50000:-50000)};
  if(next>=50000 && next<=1000000)current.camera.z=next;
}
std::pair<vec3<int32_t>,std::optional<size_t>> tactical_controller::pick(int const x,int const y,rectangle const &bounds,std::span<tactical_contact const> contacts)const{
  int const dx{x-bounds.left-((bounds.right-bounds.left)>>1)},dy{-(y-bounds.top-((bounds.bottom-bounds.top)>>1))};
  auto const &c{current.camera};
  vec3<int32_t> const point{c.x+(wrap_long(static_cast<int64_t>(dx)*c.z)>>7),c.y+(wrap_long(static_cast<int64_t>(dy)*c.z)>>7),0};
  int const radius{(wrap_long(static_cast<int64_t>(dx)*c.z)>>7)-(wrap_long(static_cast<int64_t>(dx-5)*c.z)>>7)};
  std::optional<size_t> hit;
  for(auto const &contact:contacts)if(contact.actor && contact.position.x-radius<point.x && point.x<contact.position.x+radius && contact.position.y-radius<point.y && point.y<contact.position.y+radius){hit=contact.actor;break;}
  return {vec3<int32_t>{point},hit};
}
bool tactical_controller::destination(vec3<int32_t> const &point,std::optional<size_t> const actor,std::span<tactical_contact const> contacts,vec3<int32_t> const &player,int32_t const range){
  if(current.wingmate<0)return false;
  auto &order{current.orders[static_cast<size_t>(current.wingmate)]};
  if(current.command==11 || current.command==12){
    if(actor)for(auto const &contact:contacts)if(contact.actor==actor){
      auto const &p{contact.position};auto const distance{fast_distance({p.x-player.x,p.y-player.y,p.z-player.z})};
      if(distance<range && contact.side==(current.command==11?1:0)){
        order.pending_actor=actor;order.pending_kind=1;order.pending_active=true;current.ready=true;return true;
      }
      break;
    }
    if(current.command==11 || current.ready)return false;
  }else if(current.command!=13 && current.command!=14)return false;
  order.pending=point;order.pending_kind=0;order.pending_active=true;current.ready=true;return true;
}
void tactical_controller::designate(size_t const actor){
  if(current.wingmate<0)return;
  auto &order{current.orders[static_cast<size_t>(current.wingmate)]};
  order.pending_actor=actor;order.pending_kind=1;order.pending_active=true;current.ready=true;
}
void tactical_controller::commit(){
  if(current.wingmate<0)return;
  auto &order{current.orders[static_cast<size_t>(current.wingmate)]};
  if(current.requires_destination){
    if(order.pending_kind==0)order.committed=order.pending;
    else if(order.pending_kind==1)order.committed_actor=order.pending_actor;
    order.pending_active=false;order.committed_kind=order.pending_kind;order.committed_active=true;
  }else order.committed_active=false;
  current.ready=false;
}
wingmate_order const &tactical_controller::packet(size_t const issuer){
  current.packet.command=static_cast<uint16_t>(current.command);current.packet.issuer=issuer;
  if(current.wingmate>=0){
    auto const &order{current.orders[static_cast<size_t>(current.wingmate)]};
    if(order.committed_kind==1)current.packet.target=order.committed_actor;
    else if(order.committed_kind==0)current.packet.position=order.committed;
  }
  return current.packet;
}
void tactical_controller::cancel(){
  choose(-1);if(current.wingmate>=0)current.orders[static_cast<size_t>(current.wingmate)].pending_active=false;
  wingmate(-1);current.ready=false;current.blink=false;current.requires_destination=false;
}
} // namespace earthsiege
