#include "cockpit_radio.h"
#include <algorithm>
#include <format>
#include <utility>

namespace earthsiege {
cockpit_radio::cockpit_radio(resource_store const &source,cockpit_layout const &layout,mission_session const &session,random_state &ui_random,bool const reduced_detail)
  :resources{source},random{ui_random},catalogue{source},banner{source},labels{source},parent{layout.bounds(0x4bc)}{
  auto const &world{session.activity_state()};auto const &group{world.group(world.actor(session.player_id()).group)};
  for(auto const actor:group.members){
    if(actor==session.player_id())continue;
    auto const identity{session.pilot_identity(actor)};
    if(!identity || *identity<0 || *identity>=36)throw std::runtime_error{"wingmate has no valid pilot identity"};
    auto const pilot{static_cast<size_t>(*identity/3)};auto const slot{members.size()};auto b{layout.bounds(0x5ac+slot*16)};
    b.left+=parent.left+1;b.right+=parent.left-1;b.top+=parent.top+1;b.bottom+=parent.top-1;
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
bool cockpit_radio::submit(radio_event event,uint32_t const ticks){
  auto const result{catalogue.resolve(event,speakers,random)};
  return result && controller.queue().push(result->descriptor,result->text,event,ticks);
}
void cockpit_radio::advance_portraits(mission_session &session,uint32_t const ticks){
  for(size_t i{};i<members.size();++i){
    auto &p{members[i]};auto const order{session.order_state(p.actor)};
    auto const paint{[&](int const kind,int const value){
      if(kind==3){sounds.push_back(static_cast<uint16_t>(value));return;}
      if(!visible)return;
      if(kind==0){
        if(order.dead){labels.draw_static(cache,p.painted,p.pilot,i,p.noise_frame);p.noise_frame=(p.noise_frame+1)%5;}
        else labels.draw_idle(cache,p.painted,p.pilot,i,session.instrument_damage(p.actor).armour,herc_status(order.type,order.disabled,order.dead,order.command));
      }else if(kind==1){labels.draw_static(cache,p.painted,p.pilot,i,p.noise_frame);p.noise_frame=(p.noise_frame+1)%5;}
      else if(kind==2){p.portrait.update(static_cast<size_t>(value),random);p.portrait.draw(cache,p.painted);labels.draw_name(cache,p.painted,p.pilot,i);}
      p.initialized=true;
    }};
    if(p.transmission){
      auto const update{p.transmission->advance(ticks,visible,order.dead,random)};
      if(update.ready){controller.ready();if(p.transmission->mouth()!=0)speech_requests.push_back(p.speech);}
      if(update.cancel)controller.cancel();
      for(auto const &call:update.calls)paint(call[0],call[1]);
      if(p.transmission->state().phase==0)paint(0,0);
    }else paint(0,0);
  }
}
void cockpit_radio::update(mission_session &session,camera_state const &camera,uint32_t const ticks,int const origin_x,int const origin_y){
  visible=camera.mode==camera_mode::attached && parent.right>=origin_x && parent.left<origin_x+320 && parent.bottom>=origin_y && parent.top<origin_y+200;
  for(auto &p:members){auto b{p.bounds};b.left-=origin_x;b.right-=origin_x;b.top-=origin_y;b.bottom-=origin_y;
    if(b.left!=p.painted.left || b.top!=p.painted.top){p.initialized=false;p.painted=b;}
    if(visible && !p.initialized)cache.fill(b,63);
  }
  for(auto const &reply:session.take_radio_responses())submit({reply.message,reply.actor,{},{}},ticks);
  advance_portraits(session,ticks);
  controller.advance(ticks,true,{
    .start{[&](radio_message const &message){
      auto const index{speaker_index(message.event.speaker)};
      if(!index){controller.ready();return;}
      auto &p{members[*index]};auto const filename{std::format("snc/P{}_{:02}{:03}.SNC",static_cast<char>('A'+p.pilot),message.event.id,message.flags)};
      if(!resources.archives().at("simvol0").find(filename)){controller.ready();controller.cancel();return;}
      auto timeline{read_portrait_sync(resources.read("simvol0",filename))};
      if(p.transmission)p.transmission->begin(static_cast<uint8_t>(message.event.id),std::move(timeline),ticks);
      else p.transmission.emplace(static_cast<uint8_t>(message.event.id),std::move(timeline),ticks);
      p.speech=std::format("simvoice/P{}_{:02}{:03}.SFX",speakers[*index].voice,message.event.id,message.flags);
      sounds.push_back(28);
      if(visible)advance_portraits(session,ticks);
    }},
    .show{[&](radio_message const &message){text_speaker=speaker_index(message.event.speaker);text=banner.format(text_speaker?labels.name(members[*text_speaker].pilot):"XXXX",catalogue.at(message.text).text);return true;}},
    .finish{[&](radio_message const &){text.clear();return true;}}
  });
}
void cockpit_radio::draw(framebuffer &frame)const{
  if(visible)for(auto const &p:members)if(p.initialized){
    for(int y{std::max(0,p.painted.top)};y<=std::min(199,p.painted.bottom);++y)for(int x{std::max(0,p.painted.left)};x<=std::min(319,p.painted.right);++x)
      frame.pixel(x,y,cache.pixels()[static_cast<size_t>(y*320+x)]);
  }
  if(controller.state().visible)banner.draw(frame,text,text_speaker);
}
radio_presentation_state const &cockpit_radio::state()const{return controller.state();}
std::vector<uint16_t> cockpit_radio::take_sounds(){return std::exchange(sounds,{});}
std::vector<std::string> cockpit_radio::take_speech(){return std::exchange(speech_requests,{});}
} // namespace earthsiege
