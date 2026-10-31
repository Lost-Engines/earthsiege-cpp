#include "music_audio.h"
#include "midi_synth_factory.h"
#include <algorithm>
#include <cmath>
extern "C" {
#include "third_party/nuked-opl3/opl3.h"
}
namespace earthsiege {
struct music_audio::chip_state {opl3_chip chip{};chip_state(){OPL3_Reset(&chip,44100);}};
music_audio::music_audio(resource_store const&r,pcm_sound_bank &b,music_settings const &settings):resources{r},bank{b}{
  configure(settings);
  bank.completed=[this](size_t h){player->complete_sample(static_cast<int32_t>(h));};
}
music_audio::~music_audio(){bank.completed={};}
void music_audio::configure(music_settings const &settings){
  // Create the replacement before touching live state: missing ROMs leave the
  // current song playing. A successful device change restarts that song.
  auto next_midi{make_music_synth(settings,resources.root())};
  auto next_chip{next_midi?nullptr:std::make_unique<chip_state>()};
  auto next_player{std::make_unique<music_player>(music_driver_id(settings.device),resources.read("shell0","SOS/MELODIC.BNK"),resources.read("shell0","SOS/DRUM.BNK"),resources.read("shell0","SOS/DRUM32.DIG"),static_cast<pcm_midi_backend&>(*this),[this](std::span<uint8_t const> packet){send(packet);})};
  bool const active{player&&player->playing()};if(player)player->stop();
  midi=std::move(next_midi);chip=std::move(next_chip);player=std::move(next_player);
  set_volume(volume_,master_);
  if(!song.empty()){player->load(song);player->set_loops(repeats);if(active)player->play();}
}
void music_audio::send(std::span<uint8_t const> packet){
  if(packet.empty())return;
  if(packet[0]==0xf0){midi->sysex(packet);return;}
  uint32_t packed{};for(size_t i{};i<std::min(size_t{3},packet.size());++i)packed|=static_cast<uint32_t>(packet[i])<<(8*i);
  midi->short_message(packed);
}
auto music_audio::start(midi_sample const &s)->int32_t {pcm_voice_state v;v.key=static_cast<uint16_t>(s.key);v.flags=static_cast<uint16_t>(s.flags);v.volume=static_cast<uint16_t>(s.volume);v.pan=static_cast<uint16_t>(s.pan);v.loops=s.loops;v.samples=s.samples;return bank.start_voice(v);}
void music_audio::stop(int32_t h){bank.stop_voice(static_cast<size_t>(h));}
void music_audio::volume(int32_t h,uint32_t value){bank.voice_volume(static_cast<size_t>(h),static_cast<uint16_t>(value));}
void music_audio::pan(int32_t h,uint32_t value){bank.voice_pan(static_cast<size_t>(h),static_cast<uint16_t>(value));}
void music_audio::load(bytes data,uint16_t loops){song.assign(data.begin(),data.end());repeats=loops;player->load(song);player->set_loops(loops);}
void music_audio::play(){player->play();}
void music_audio::stop(){player->stop();}
bool music_audio::playing()const{return player->playing();}
void music_audio::set_volume(uint8_t value,uint8_t master){volume_=value;master_=master;player->set_master(static_cast<uint8_t>(static_cast<uint32_t>(value)*127*master/10000));}
void music_audio::tick(){player->tick();}
auto music_audio::render(size_t frames)->std::vector<int16_t> {
  std::vector<int16_t> out(frames*2);
  if(midi){
    std::vector<float> pcm(frames*2);midi->render(pcm);
    for(size_t i{};i<pcm.size();++i)out[i]=static_cast<int16_t>(std::clamp(std::isfinite(pcm[i])?pcm[i]*32768.0f:0.0f,-32768.0f,32767.0f));
  }else{
    for(auto const &w:player->take_registers())OPL3_WriteRegBuffered(&chip->chip,w.reg,w.value);
    for(size_t i{};i<frames;++i)OPL3_GenerateResampled(&chip->chip,out.data()+i*2);
  }
  return out;
}
}
