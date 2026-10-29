#include "hmp_player.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
hmp_player::hmp_player(hmp_song const &source,std::span<uint32_t const> installed)
  : song{source},devices{installed.begin(),installed.end()} { restart(); }
void hmp_player::read_delta(size_t const index) {
  auto &state{tracks.at(index)};binary_reader f{song.data,"HMP delta"};f.seek(state.cursor);
  state.delta=read_hmp_delta(f);state.cursor=static_cast<uint32_t>(f.position());
}
void hmp_player::restart() {
  active=true;remaining=0;tracks.clear();
  for(auto const &source:song.tracks) {
    track_state state;state.cursor=source.offset+12;
    if(source.devices.front()==0 && !devices.empty())state.device=0;
    else for(auto const candidate:source.devices) {
      if(!candidate || state.device)break;
      for(size_t i{};i<devices.size();++i) {
        auto const id{devices[i]};bool const match{candidate==id || (candidate==0xa000 && (id==0xa001 || id==0xa008)) || (candidate==0xa002 && id==0xa009)};
        if(match){state.device=i;break;}
      }
    }
    state.active=state.device.has_value();if(state.active)++remaining;
    for(auto const &b:source.branches)state.loops.push_back(b.loops);
    tracks.push_back(std::move(state));read_delta(tracks.size()-1);
  }
  if(!remaining)active=false;
}
void hmp_player::stop(){active=false;}
bool hmp_player::playing()const{return active;}
std::vector<std::optional<size_t>> hmp_player::routing()const {
  std::vector<std::optional<size_t>> result;for(auto const &state:tracks)result.push_back(state.device);return result;
}
size_t hmp_player::branch(size_t const index,uint8_t const id)const {
  auto const &entries{song.tracks.at(index).branches};
  for(size_t i{};i<entries.size();++i)if(entries[i].id==id)return i;
  throw std::runtime_error{"HMP references an absent branch"};
}
void hmp_player::jump(size_t const index,size_t const selected,bool const global,std::vector<midi_packet> &packets) {
  auto &state{tracks.at(index)};auto const &source{song.tracks[index]};auto const &entry{source.branches.at(selected)};
  state.cursor=source.offset+12+entry.event_offset;state.elapsed=0;
  uint8_t channel;
  if(global){read_delta(index);channel=song.data.at(state.cursor)&15;}
  else channel=static_cast<uint8_t>(source.channel);
  auto const device{devices.at(*state.device)};
  packets.push_back({device,{static_cast<uint8_t>(channel|0xc0),entry.program},{}});
  for(size_t i{};i<entry.controls.size();i+=2)packets.push_back({device,{static_cast<uint8_t>(channel|0xb0),entry.controls[i],entry.controls[i+1]},{}});
}
std::vector<midi_packet> hmp_player::tick() {
  std::vector<midi_packet> packets;if(!active)return packets;
  size_t event_budget{100000};
  for(size_t i{};i<tracks.size();++i) {
    auto &state{tracks[i]};++state.elapsed;
    if(!state.active || state.delta>state.elapsed)continue;
    do {
      if(!event_budget--)throw std::runtime_error{"HMP has an unbounded zero-time loop"};
      auto const offset{state.cursor};auto const status{song.data.at(offset)};
      auto const extent{static_cast<uint64_t>(song.tracks[i].offset)+song.tracks[i].length};
      size_t length{(status>>4==12 || status>>4==13) ? 2U : 3U};
      if(status<0x80 || offset+length>extent)throw std::runtime_error{"invalid HMP event"};
      bool send{true},consume_delta{true};
      if(status==0xff) {
        if(song.data.at(offset+1)!=0x2f || song.data.at(offset+2)!=0)throw std::runtime_error{"unsupported HMP meta event"};
        state.active=false;send=false;if(--remaining==0){active=false;return packets;}
      }else if(status>=0xf0)throw std::runtime_error{"unsupported HMP system event"};
      else if((status&0xf0)==0xb0) {
        auto const controller{song.data.at(offset+1)},value{song.data.at(offset+2)};
        if(controller>=108 && controller<=121) {
          send=false;
          if(controller==109 || controller==115)state.loops.at(branch(i,value))=song.data.at(offset+6);
          else if(controller==111 || controller==112 || controller==114 || controller==117 || controller==118 || controller==121) {
            auto const id{static_cast<uint8_t>(controller==114 ? value|128 : value)};auto const selected{branch(i,id)};
            auto &count{state.loops.at(selected)};bool take{true};
            if(controller!=114 && controller!=121){if(count && count!=255)--count;take=count!=0;if(loop_filter&&!loop_filter(id))take=false;}
            if(take) {
              bool const global{controller==111 || controller==112 || controller==114};
              if(global) {for(size_t t{1};t<tracks.size();++t)if(tracks[t].active)jump(t,branch(t,id),true,packets);consume_delta=false;}
              else jump(i,selected,false,packets);
              length=0;
            }
          }
          // Controllers 108/110/113/116/120 are markers. 119 has no callback
          // in Earthsiege's normal driver setup, so only consumes its event.
        }
      }
      if(send)packets.push_back({devices.at(*state.device),{song.data.begin()+offset,song.data.begin()+offset+length},offset});
      state.elapsed=0;if(!state.active)break;
      state.cursor+=static_cast<uint32_t>(length);if(consume_delta)read_delta(i);
    }while(state.delta==0);
  }
  return packets;
}
} // namespace earthsiege
