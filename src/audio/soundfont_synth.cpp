#include "soundfont_synth.h"
#include <tsf.h>
#include <stdexcept>
namespace earthsiege {
struct soundfont_synth::implementation {std::unique_ptr<tsf,decltype(&tsf_close)> synth{nullptr,tsf_close};};
soundfont_synth::soundfont_synth(std::filesystem::path const &path,unsigned rate):state{std::make_unique<implementation>()}{
  state->synth.reset(tsf_load_filename(path.string().c_str()));
  if(!state->synth)throw std::runtime_error{"cannot load SoundFont: "+path.string()};
  tsf_set_output(state->synth.get(),TSF_STEREO_INTERLEAVED,static_cast<int>(rate),-6.0f);
  if(!tsf_set_max_voices(state->synth.get(),256))throw std::runtime_error{"cannot allocate SoundFont voices"};
  for(int ch{};ch<16;++ch)if(!tsf_channel_set_presetnumber(state->synth.get(),ch,0,ch==9))throw std::runtime_error{"SoundFont needs melodic and percussion presets"};
}
soundfont_synth::~soundfont_synth()=default;
void soundfont_synth::short_message(uint32_t packed){
  auto *s{state->synth.get()};int const ch{static_cast<int>(packed&15)},first{static_cast<int>((packed>>8)&127)},second{static_cast<int>((packed>>16)&127)};
  switch(packed&0xf0){
    case 0x80:tsf_channel_note_off(s,ch,first);break;
    case 0x90:if(second)tsf_channel_note_on(s,ch,first,static_cast<float>(second)/127);else tsf_channel_note_off(s,ch,first);break;
    case 0xb0:tsf_channel_midi_control(s,ch,first,second);break;
    case 0xc0:tsf_channel_set_presetnumber(s,ch,first,ch==9);break;
    case 0xe0:tsf_channel_set_pitchwheel(s,ch,first|(second<<7));break;
    default:break;
  }
}
void soundfont_synth::sysex(std::span<uint8_t const>){ /* Supplied HMPs contain no system-exclusive events. */ }
void soundfont_synth::render(std::span<float> stereo){
  if(stereo.size()%2)throw std::invalid_argument{"SoundFont output needs complete stereo frames"};
  tsf_render_float(state->synth.get(),stereo.data(),static_cast<int>(stereo.size()/2),0);
}
}
