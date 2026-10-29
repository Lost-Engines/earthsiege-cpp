#include "audio/mission_audio.h"
#include "shell/cockpit_radio.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include <algorithm>
#include <iostream>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=2)return 1;
  resource_store resources{argv[1]};size_t effect_samples{},speech_samples{};
  for(size_t id{10};id<=46;++id){
    mission_audio audio{resources};if(!audio.play(id))throw std::runtime_error{"effect failed to start: "+std::to_string(id)};
    audio.advance(360);auto const pcm{audio.take_samples()};auto const audible{std::ranges::count_if(pcm,[](uint8_t sample){return sample!=128;})};
    if(!audible)throw std::runtime_error{"silent effect: "+std::to_string(id)};
    if(pcm.size()!=3*11025*2)throw std::runtime_error{"PCM clock duration"};
    effect_samples+=static_cast<size_t>(audible);
  }
  auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  for(unsigned int demo{1};demo<=4;++demo){
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    mission_session session{resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const cockpit{read_cockpit(resources,names.at(static_cast<size_t>(session.player_chassis())))};random_state random{17};cockpit_radio radio{resources,cockpit.layout,session,random};mission_audio audio{resources};
    auto camera{session.make_camera(camera_mode::attached)};auto const origin{cockpit.windows[static_cast<size_t>(cockpit_view::lower)].instrument_origin};session.transmit_order(4);size_t spoken{};
    for(uint32_t tick{};tick<600;++tick){
      radio.update(session,camera,tick,origin[0],origin[1]);
      for(auto const id:radio.take_sounds())audio.play(id);
      for(auto const &path:radio.take_speech()){if(!audio.speech(path))throw std::runtime_error{"speech did not start: "+path};++spoken;}
      audio.advance(2);auto const pcm{audio.take_samples()};speech_samples+=static_cast<size_t>(std::ranges::count_if(pcm,[](uint8_t sample){return sample!=128;}));
    }
    if(spoken!=1)throw std::runtime_error{"live radio speech count"};
    for(size_t slot{47};slot<57;++slot)if(audio.playing(slot))throw std::runtime_error{"speech voice did not complete"};
  }
  if(!speech_samples)throw std::runtime_error{"silent radio"};
  // Live encounter decisions drive real song loading, FM and shared PCM voices.
  mission_audio soundtrack{resources};random_state rng;music_contacts encounters;
  bool heard{},percussion{},combat{},returned{};size_t output_samples{};
  for(unsigned int step{};step<600;++step){
    soundtrack.advance();encounters.actors.clear();if(step>=200&&step<300)encounters.actors.push_back({1000,1,false,false,false,true});
    soundtrack.update_music(encounters,rng);auto const track{soundtrack.music_status().track};if(track>=3)combat=true;else if(combat)returned=true;
    auto const output{soundtrack.take_output()};output_samples+=output.size();heard=heard||std::ranges::any_of(output,[](int16_t v){return v!=0;});
    for(auto const &v:soundtrack.state().voices())if((v.flags&0x8000)&&v.key<128)percussion=true;
  }
  if(!heard||!percussion||!combat||!returned||output_samples!=15*44100*2)throw std::runtime_error{"live combat music integration"};
  std::cout<<"37 effects and four live wingmate acknowledgements: "<<effect_samples<<" effect / "<<speech_samples<<" speech non-silent samples\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
