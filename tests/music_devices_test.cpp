#include "audio/music_audio.h"
#include "audio/midi_synth_factory.h"
#include "audio/shell_audio.h"
#include "audio/mission_audio.h"
#include "shell/save_store.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,char const *message){if(!value)throw std::runtime_error{message};}
long long energy(std::span<int16_t const> pcm){long long result{};for(auto sample:pcm)result+=std::abs(static_cast<int>(sample));return result;}
}
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=3&&argc!=5)return 2;
 resource_store resources{argv[1]};auto const root{std::filesystem::path{argv[2]}/("music-settings-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
 struct cleanup {std::filesystem::path path;~cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}} clean{root};
 require(read_music_settings(root/"music.cfg").device==music_device::opl3,"default music changed");
 music_settings settings;settings.device=music_device::mt32_mpu;settings.rom_directory=root/"ROM files";settings.soundfont=root/"piano bank.sf2";
 write_music_settings(root/"music.cfg",settings);auto restored{read_music_settings(root/"music.cfg")};
 require(restored.device==settings.device&&restored.rom_directory==settings.rom_directory&&restored.soundfont==settings.soundfont,"music settings round trip");
 save_store{root}.write("broken.cfg",std::array<uint8_t,7>{'d','e','v','i','c','e',' '});bool rejected{};try{read_music_settings(root/"broken.cfg");}catch(std::runtime_error const&){rejected=true;}require(rejected,"truncated settings accepted");
 auto dirs{music_asset_directories(settings,resources.root())};require(dirs.size()==1&&dirs.front()==settings.rom_directory,"explicit ROM directory ignored");
 // A choice made before any song starts must still reject missing assets,
 // otherwise the UI can persist a configuration that fails on the next launch.
 settings={};settings.device=music_device::sc55;settings.rom_directory=root/"absent";
 shell_audio idle_shell{resources};mission_audio idle_mission{resources};
 rejected=false;try{idle_shell.configure_music(settings);}catch(std::runtime_error const&){rejected=true;}
 require(rejected,"unstarted shell accepted missing ROMs");
 rejected=false;try{idle_mission.configure_music(settings);}catch(std::runtime_error const&){rejected=true;}
 require(rejected,"unstarted mission accepted missing ROMs");
 idle_shell.song(1);idle_shell.advance(240);require(energy(idle_shell.take_output())>100000,"failed initial choice changed shell settings");
 for(auto mode:{music_device::opl2,music_device::opl3}){
   settings={};settings.device=mode;shell_audio shell{resources,settings};shell.song(1);shell.advance(240);require(energy(shell.take_output())>100000,"FM music silent");
   auto bad{settings};bad.device=music_device::sc55;bad.rom_directory=root/"absent";rejected=false;
   try{shell.configure_music(bad);}catch(std::runtime_error const&){rejected=true;}
   require(rejected,"missing ROMs not reported");shell.advance(120);require(energy(shell.take_output())>100000,"failed switch interrupted music");
   settings.device=mode==music_device::opl3?music_device::opl2:music_device::opl3;shell.configure_music(settings);shell.advance(240);require(energy(shell.take_output())>100000,"live FM switch silent");
 }
 if(argc==5){
   for(auto mode:{music_device::mt32,music_device::mt32_mpu,music_device::sc55,music_device::scc1a,music_device::soundfont}){
     std::cout<<"Checking "<<music_device_name(mode)<<std::endl;
     settings={};settings.device=mode;settings.rom_directory=argv[3];settings.soundfont=argv[4];
     // Establish that each real chip/backend receives MIDI and emits sound,
     // including MT-32 whose authentic Earthsiege routing has no melodic parts.
     auto synth{make_music_synth(settings,resources.root())};
     synth->short_message(0x0000c1);synth->short_message(0x7f07b1);synth->short_message(0x643c91);
     std::vector<float> note(44100);synth->render(note);
     double sum{};for(float sample:note){require(std::isfinite(sample),"non-finite synth PCM");sum+=std::abs(sample);}require(sum>0.1,"hardware note silent");
     synth.reset();
     shell_audio shell{resources,settings};shell.song(1);shell.advance(240);auto pcm{shell.take_output()};require(pcm.size()==44100*4,"music clock output length");require(energy(pcm)>10000,"music plus percussion silent");
     shell.suspend();shell.advance(120);shell.take_output();shell.resume();shell.advance(240);auto resumed{shell.take_output()};
     std::cout<<"energy start/resume "<<energy(pcm)<<" / "<<energy(resumed)<<std::endl;
     require(energy(resumed)>10000,"hardware resume silent");
     settings.device=music_device::opl3;shell.configure_music(settings);shell.advance(120);require(energy(shell.take_output())>10000,"switch from hardware silent");
     std::cout<<music_device_name(mode)<<": firmware/SoundFont output, original song, suspend/resume and switching passed\n";
   }
 }
 std::cout<<"Saved selection, explicit asset paths, missing-ROM recovery and OPL switching passed\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
