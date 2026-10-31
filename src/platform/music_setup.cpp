#include "music_setup.h"
#include <SDL.h>
#include <array>
#include <span>
#include <stdexcept>
#include <string>
namespace earthsiege {
namespace {
int choose(std::string const &message,std::span<SDL_MessageBoxButtonData const> buttons){
  SDL_MessageBoxData box{};box.flags=SDL_MESSAGEBOX_INFORMATION;box.title="Earthsiege music hardware";
  box.message=message.c_str();box.numbuttons=static_cast<int>(buttons.size());box.buttons=buttons.data();
  int result{-1};if(SDL_ShowMessageBox(&box,&result)!=0)throw std::runtime_error{SDL_GetError()};return result;
}
constexpr Uint32 cancel{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT};
}
std::optional<music_device> choose_music_device(music_device current){
  for(;;){
    std::array<SDL_MessageBoxButtonData,4> choices{{{cancel,-1,"Cancel"},{0,0,"Sound Blaster / AdLib"},{0,1,"Roland"},{0,2,"SoundFont"}}};
    int const group{choose("Current: "+std::string{music_device_label(current)}+"\nChoose a music device. Sound effects and speech keep their current settings.\nChanges restart the current song and are saved for future launches.",choices)};
    if(group<0)return {};
    if(group==2)return music_device::soundfont;
    if(group==0){
      std::array<SDL_MessageBoxButtonData,3> fm{{{cancel,-1,"Back"},{0,0,"OPL2"},{0,1,"OPL3"}}};
      int const choice{choose("OPL2: AdLib / Sound Blaster\nOPL3: Sound Blaster Pro / 16\nBoth use Earthsiege's original FM instruments and sampled percussion.",fm)};
      if(choice>=0)return choice?music_device::opl3:music_device::opl2;
    }else{
      std::array<SDL_MessageBoxButtonData,4> roland{{{cancel,-1,"Back"},{0,0,"SC-55"},{0,1,"SCC-1A"},{0,2,"MT-32 / CM-32L"}}};
      int const choice{choose("Sound Canvas plays the game's General MIDI arrangement.\nRoland hardware emulation requires matching ROMs.\nThe fetch-assets script installs them in the default location.",roland)};
      if(choice==0)return music_device::sc55;
      if(choice==1)return music_device::scc1a;
      if(choice==2){
        std::array<SDL_MessageBoxButtonData,3> la{{{cancel,-1,"Back"},{0,0,"Original MT-32 selection"},{0,1,"MPU-401 experiment"}}};
        int const mode{choose("This release has no tracks assigned to its MT-32 driver.\nThe original selection therefore plays only any sampled percussion.\n\nMPU-401 sends the General MIDI parts to the MT-32 instead.\nIts instrument numbering differs, so this is a listening experiment.",la)};
        if(mode>=0)return mode?music_device::mt32_mpu:music_device::mt32;
      }
    }
  }
}
}
