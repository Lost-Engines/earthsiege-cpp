#include "midi_synth_factory.h"
#include "la_synth.h"
#include "sc55_synth.h"
#include "soundfont_synth.h"
#include <stdexcept>
namespace earthsiege {
std::unique_ptr<midi_receiver> make_music_synth(music_settings const &s,std::filesystem::path const &data,unsigned rate){
  if(s.device==music_device::opl2||s.device==music_device::opl3)return {};
  auto const paths{music_asset_directories(s,data)};
  if(s.device==music_device::soundfont){
    if(!s.soundfont.empty())return std::make_unique<soundfont_synth>(s.soundfont,rate);
    for(auto const &path:paths)for(auto const name:{"default.sf2","default-GM.sf2","FluidR3_GM.sf2","TimGM6mb.sf2"})if(std::filesystem::is_regular_file(path/name))return std::make_unique<soundfont_synth>(path/name,rate);
    for(auto const name:{"/usr/share/sounds/sf2/default-GM.sf2","/usr/share/soundfonts/default.sf2"})if(std::filesystem::is_regular_file(name))return std::make_unique<soundfont_synth>(name,rate);
    throw std::runtime_error{"No General MIDI SoundFont found. Put default.sf2 beside the game data, or use --soundfont PATH."};
  }
  std::string errors;
  for(auto const &path:paths){
    if(!std::filesystem::is_directory(path))continue;
    try{
      if(s.device==music_device::mt32||s.device==music_device::mt32_mpu)return std::make_unique<la_synth>(path,rate);
      return std::make_unique<sc55_synth>(path,rate,s.device==music_device::scc1a?sound_canvas_model::scc1a:sound_canvas_model::sc55);
    }catch(std::runtime_error const &error){errors+=std::string{"\n"}+error.what();}
  }
  throw std::runtime_error{std::string{music_device_label(s.device)}+": ROMs unavailable. Run scripts/fetch-assets.sh (or fetch-assets.bat), or use --rom-dir PATH."+errors};
}
}
