#include "music_audio.h"
extern "C" {
#include "third_party/nuked-opl3/opl3.h"
}
namespace earthsiege {
struct music_audio::chip_state {opl3_chip chip{};chip_state(){OPL3_Reset(&chip,44100);}};
music_audio::music_audio(resource_store const&r,pcm_sound_bank &b,bool opl3):bank{b},player{opl3,r.read("shell0","SOS/MELODIC.BNK"),r.read("shell0","SOS/DRUM.BNK"),r.read("shell0","SOS/DRUM32.DIG"),*this},chip{std::make_unique<chip_state>()}{bank.completed=[this](size_t h){player.complete_sample(static_cast<int32_t>(h));};set_volume(100);}
music_audio::~music_audio(){bank.completed={};}
auto music_audio::start(midi_sample const &s)->int32_t {pcm_voice_state v;v.key=static_cast<uint16_t>(s.key);v.flags=static_cast<uint16_t>(s.flags);v.volume=static_cast<uint16_t>(s.volume);v.pan=static_cast<uint16_t>(s.pan);v.loops=s.loops;v.samples=s.samples;return bank.start_voice(v);}
void music_audio::stop(int32_t h){bank.stop_voice(static_cast<size_t>(h));}
void music_audio::volume(int32_t h,uint32_t value){bank.voice_volume(static_cast<size_t>(h),static_cast<uint16_t>(value));}
void music_audio::pan(int32_t h,uint32_t value){bank.voice_pan(static_cast<size_t>(h),static_cast<uint16_t>(value));}
void music_audio::load(bytes song,uint16_t loops){player.load(song);player.set_loops(loops);}
void music_audio::play(){player.play();}
void music_audio::stop(){player.stop();}
bool music_audio::playing()const{return player.playing();}
void music_audio::set_volume(uint8_t value,uint8_t master){player.set_master(static_cast<uint8_t>(static_cast<uint32_t>(value)*127*master/10000));}
void music_audio::tick(){player.tick();}
auto music_audio::render(size_t frames)->std::vector<int16_t> {
  for(auto const &w:player.take_registers())OPL3_WriteRegBuffered(&chip->chip,w.reg,w.value);
  std::vector<int16_t> out(frames*2);for(size_t i{};i<frames;++i)OPL3_GenerateResampled(&chip->chip,out.data()+i*2);return out;
}
}
