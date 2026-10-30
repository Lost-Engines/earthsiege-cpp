#include "cockpit_radio.h"
#include <algorithm>
#include <format>
#include <utility>

namespace earthsiege {
cockpit_radio::cockpit_radio(resource_store const &source,cockpit_layout const &layout,mission_session const &session,random_state &ui_random,bool const reduced_detail)
  :resources{source},random{ui_random},catalogue{source},system_bounds{layout.bounds(0x694)},banner{source},labels{source},parent{layout.bounds(0x4bc)}{
  for(size_t slot{};slot<portrait_bounds.size();++slot){
    auto b{layout.bounds(0x5ac+slot*16)};
    b.left+=parent.left+1;b.right+=parent.left-1;b.top+=parent.top+1;b.bottom+=parent.top-1;
    portrait_bounds[slot]=b;
  }
  auto const &world{session.activity_state()};auto const &group{world.group(world.actor(session.player_id()).group)};
  for(auto const actor:group.members){
    if(actor==session.player_id())continue;
    auto const identity{session.pilot_identity(actor)};
    if(!identity || *identity<0 || *identity>=36)throw std::runtime_error{"wingmate has no valid pilot identity"};
    auto const pilot{static_cast<size_t>(*identity/3)};auto const slot{members.size()};auto const b{portrait_bounds[slot]};
    members.push_back({actor,pilot,b,b,radio_portrait{source,pilot,reduced_detail},{},0,false,{}});
    auto const voice{static_cast<uint8_t>(pilot/4+1)};speakers.push_back({actor,static_cast<uint8_t>(voice==3?4:voice),actor==group.members.front()});
    if(members.size()==3)break;
  }
  auto b{layout.bounds(0x4cc)};b.left+=parent.left;b.right+=parent.left;b.top+=parent.top;b.bottom+=parent.top;parent=b;
}
std::optional<size_t> cockpit_radio::speaker_index(std::optional<size_t> const actor)const{
  if(!actor)return {};
  for(size_t i{};i<members.size();++i)if(members[i].actor==*actor)return i;
  return {};
}
void cockpit_radio::play_sound(uint16_t id,bool idle_only){if(sound_player)sound_player(id,idle_only);else sounds.push_back(id);}
uint32_t cockpit_radio::read_input(radio_input_point point,uint32_t fallback)const{return observation?observation(point,fallback):fallback;}
bool cockpit_radio::submit(radio_event event,uint32_t const ticks){
  if(event.cancel)return controller.cancel_message(event.id,event.speaker);
  auto const result{catalogue.resolve(event,speakers,random)};
  if(!result)return false;
  auto &destination{event.speaker?controller:system_controller};
  return destination.queue().push(result->descriptor,result->text,event,read_input(radio_input_point::enqueue,ticks));
}
void cockpit_radio::advance_portraits(mission_session &session,uint32_t const fallback){
  auto const ticks{read_input(radio_input_point::portraits,fallback)};
  auto const animation_ticks{read_input(radio_input_point::animation,ticks)};
  for(size_t i{};i<members.size();++i){
    auto &p{members[i]};auto const order{session.order_state(p.actor)};
    auto const paint{[&](int const kind,int const value){
      if(kind==3){play_sound(static_cast<uint16_t>(value),true);return;}
      if(!visible)return;
      if(kind==0){
        if(order.dead){labels.draw_static(cache,p.painted,p.pilot,i,p.noise_frame);p.noise_frame=(p.noise_frame+1)%5;}
        else labels.draw_idle(cache,p.painted,p.pilot,i,session.instrument_damage(p.actor).armour,herc_status(order.type,order.disabled,order.dead,order.command));
      }else if(kind==1){labels.draw_static(cache,p.painted,p.pilot,i,p.noise_frame);p.noise_frame=(p.noise_frame+1)%5;}
      else if(kind==2){p.portrait.update(static_cast<size_t>(value),random);p.portrait.draw(cache,p.painted);labels.draw_name(cache,p.painted,p.pilot,i);}
      p.initialized=true;
    }};
    if(p.transmission){
      auto const update{p.transmission->advance(ticks,visible,order.dead,random,animation_ticks,[&](int16_t mouth){
        bool const accepted{mouth!=0&&speech_player?speech_player(p.speech):true};
        if(mouth!=0&&!speech_player)speech_requests.push_back(p.speech);
        return read_input(radio_input_point::speech_playback,accepted)!=0;
      })};
      if(update.ready)controller.ready();
      if(update.cancel)controller.cancel();
      for(auto const &call:update.calls)paint(call[0],call[1]);
      if(p.transmission->state().phase==0)paint(0,0);
    }else paint(0,0);
  }
}
void cockpit_radio::prepare(mission_session &session,camera_state const &camera,uint32_t const ticks,int const origin_x,int const origin_y,bool const restoring_cockpit){
  bool const inside{camera.mode==camera_mode::attached||restoring_cockpit};
  visible=inside && parent.right>=origin_x && parent.left<origin_x+320 && parent.bottom>=origin_y && parent.top<origin_y+200;
  for(auto &p:members){auto b{p.bounds};b.left-=origin_x;b.right-=origin_x;b.top-=origin_y;b.bottom-=origin_y;
    if(b.left!=p.painted.left || b.top!=p.painted.top){p.initialized=false;p.painted=b;}
    if(visible && !p.initialized)cache.fill(b,63);
  }
  for(auto event:session.take_radio_events())submit(event,ticks);
  advance_portraits(session,ticks);
  // 2fab8 forces system captions on and marks readiness only after advancing.
  // These alerts have their own queue and cannot interrupt a pilot transmission.
  system_enabled=inside;system_origin_x=origin_x;system_origin_y=origin_y;
  if(system_enabled){
  system_ticks=read_input(radio_input_point::system_queue,ticks);
  system_controller.advance(system_ticks,true,{
    .start{[](radio_message const &){}},
    .show{[&](radio_message const &message){
      system_painted=true;system_text=catalogue.at(message.text).text.substr(0,80);
      switch(message.event.id){
        case 6:case 14:play_sound(25);break;
        case 8:case 11:case 12:case 13:case 15:case 17:case 19:case 20:play_sound(24);break;
        case 16:case 18:case 27:case 28:play_sound(23);break;
        case 31:system_blink=true;play_sound(26);break;
        case 32:case 33:system_blink=false;play_sound(26);break;
        default:if(message.event.id<=30)play_sound(26);break;
      }
      return true;
    }},
    .finish{[&](radio_message const &){system_text.clear();return true;}}
  });
  if(system_controller.state().starting)system_controller.ready();
  }
}
void cockpit_radio::update(mission_session &session,camera_state const &camera,uint32_t ticks,int origin_x,int origin_y,bool restoring_cockpit){
  prepare(session,camera,ticks,origin_x,origin_y,restoring_cockpit);
  advance_queue(session,ticks);
}
void cockpit_radio::advance_queue(mission_session &session,uint32_t ticks){
  controller.advance(read_input(radio_input_point::pilot_queue,ticks),text_enabled,{
    .start{[&](radio_message const &message){
      auto const index{speaker_index(message.event.speaker)};
      if(!index){controller.ready();return;}
      auto &p{members[*index]};auto const filename{std::format("snc/P{}_{:02}{:03}.SNC",static_cast<char>('A'+p.pilot),message.event.id,message.flags)};
      if(!resources.archives().at("simvol0").find(filename)){controller.ready();controller.cancel();return;}
      auto timeline{read_portrait_sync(resources.read("simvol0",filename))};
      auto const begin{read_input(radio_input_point::transmission,ticks)};
      if(p.transmission)p.transmission->begin(static_cast<uint8_t>(message.event.id),std::move(timeline),begin);
      else p.transmission.emplace(static_cast<uint8_t>(message.event.id),std::move(timeline),begin);
      p.speech=std::format("simvoice/P{}_{:02}{:03}.SFX",speakers[*index].voice,message.event.id,message.flags);
      play_sound(28,true);
      if(visible)advance_portraits(session,ticks);
    }},
    .show{[&](radio_message const &message){text_speaker=speaker_index(message.event.speaker);text=banner.format(text_speaker?labels.name(members[*text_speaker].pilot):"XXXX",catalogue.at(message.text).text);return true;}},
    .finish{[&](radio_message const &){text.clear();return true;}}
  });

}
void cockpit_radio::draw(framebuffer &frame)const{
  if(visible){
    auto bounds{portrait_bounds};
    for(auto &b:bounds){b.left-=system_origin_x;b.right-=system_origin_x;b.top-=system_origin_y;b.bottom-=system_origin_y;}
    wingmate_portrait::draw_empty(frame,bounds,(1u<<members.size())-1);
  }
  if(visible)for(auto const &p:members)if(p.initialized){
    for(int y{std::max(0,p.painted.top)};y<=std::min(199,p.painted.bottom);++y)for(int x{std::max(0,p.painted.left)};x<=std::min(319,p.painted.right);++x)
      frame.pixel(x,y,cache.pixels()[static_cast<size_t>(y*320+x)]);
  }
  if(controller.state().visible)banner.draw(frame,text,text_speaker);
  // Original 2fb18 erases an expired system message to colour 63. The
  // retained cockpit page keeps that blank region until its next full repaint.
  if(system_enabled&&system_painted){
    auto bounds{system_bounds};bounds.left-=system_origin_x;bounds.right-=system_origin_x;
    bounds.top-=system_origin_y;bounds.bottom-=system_origin_y;
    banner.draw_system(frame,system_text,bounds,system_controller.state().visible&&(!system_blink||(system_ticks&32)));
  }
}
radio_presentation_state const &cockpit_radio::state()const{return controller.state();}
std::vector<uint16_t> cockpit_radio::take_sounds(){return std::exchange(sounds,{});}
std::vector<std::string> cockpit_radio::take_speech(){return std::exchange(speech_requests,{});}
} // namespace earthsiege
