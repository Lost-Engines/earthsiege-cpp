#include "combat_music.h"
#include <algorithm>
namespace earthsiege {
auto advance_combat_music(combat_music_state &s,music_playback &p,std::span<music_contact const> actors,std::span<music_contact const> flyers,uint32_t clock,bool enabled,bool driver,random_state &random)->std::vector<music_request> {
  std::vector<music_request> out;if(!enabled||!driver)return out;
  auto request=[&](music_action action,uint8_t value=0){out.push_back({action,static_cast<uint8_t>(s.track),value});};
  if(s.track==-1){s.track=random.bounded(3);request(music_action::load);s.battle=false;s.deadline=clock+300;}
  if(s.fading){
    auto &v{p.volume.at(static_cast<size_t>(s.track))};
    if(v){v=static_cast<uint8_t>(std::max(0,static_cast<int>(v)-15));request(music_action::volume,v);return out;}
    p.playing[static_cast<size_t>(s.track)]=false;request(music_action::stop);s.track=static_cast<int16_t>(s.track+(s.battle?3:-3));request(music_action::load);p.volume.at(static_cast<size_t>(s.track))=100;request(music_action::volume,100);s.fading=false;
  }
  if(s.deadline<clock){
    for(auto const &a:actors)if(!a.disabled&&!a.dead&&!a.waiting&&a.side==1&&(a.distance<25000||a.visible)){
      if(s.battle)return out;
      s.fading=true;s.battle=true;s.deadline=clock+300;break;
    }
    // Preserve the original distant-flyer test; it does not inspect allegiance.
    if(s.battle&&!s.fading){s.fading=std::ranges::none_of(flyers,[](auto const &a){return !a.dead&&!a.disabled&&a.distance>18928;});if(s.fading){s.battle=false;s.deadline=clock+300;}}
  }
  if(!p.playing.at(static_cast<size_t>(s.track))){p.playing[static_cast<size_t>(s.track)]=true;request(music_action::play);}return out;
}
}
