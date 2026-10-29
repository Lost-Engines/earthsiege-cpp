#include "music_player.h"
namespace earthsiege {
music_player::music_player(bool opl3,bytes melodic,bytes drums,bytes digital_bank,pcm_midi_backend &backend)
 :devices{opl3?0xa009u:0xa002u,0xa005},fm{opl3,melodic,drums},pcm{digital_bank,backend},gain{devices}{}
void music_player::load(bytes hmp){stop();scheduler.reset();song.emplace(read_hmp(hmp));scheduler=std::make_unique<hmp_player>(*song,devices);scheduler->loop_filter=[this](uint8_t id){return !loops||loops->allow(id,playing_);};}
void music_player::set_loops(uint16_t count){if(!loops)loops.emplace();if(playing_)loops->remaining=count;else loops->configured=count;}
void music_player::play(){if(scheduler&&!playing_){scheduler->restart();if(loops)loops->remaining=loops->configured;playing_=true;}}
void music_player::dispatch(midi_packet const &packet){if(packet.device==devices[0])fm.midi(packet.wire);else if(packet.device==devices[1])pcm.midi(packet.wire);}
void music_player::cleanup(){
  auto const routes{scheduler->routing()};
  for(size_t i{};i<routes.size();++i)if(routes[i]){
    auto const ch{static_cast<uint8_t>(song->tracks[i].channel)};auto const device{devices[*routes[i]]};
    dispatch({device,{static_cast<uint8_t>(ch|0xb0),123,0},{}});dispatch({device,{static_cast<uint8_t>(ch|0xb0),121,0},{}});
    dispatch({device,{static_cast<uint8_t>(ch|0xe0),64,64},{}});dispatch({device,{static_cast<uint8_t>(ch|0xb0),7,0},{}});
  }
}
void music_player::stop(){if(playing_){cleanup();playing_=false;scheduler->stop();}}
void music_player::tick(){
  if(!playing_)return;
  for(auto packet:scheduler->tick())dispatch(gain.apply(std::move(packet)));
  if(!scheduler->playing()){cleanup();playing_=false;}
}
void music_player::set_master(uint8_t value){for(auto const &p:gain.set_master(value))dispatch(p);}
void music_player::complete_sample(int32_t handle){pcm.complete(handle);}
auto music_player::take_registers()->std::vector<opl_write>{return fm.take_writes();}
}
