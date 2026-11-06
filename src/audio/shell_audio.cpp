#include "shell_audio.h"
#include <utility>
namespace earthsiege {
namespace {
// GO 0x828c0: retain IDs, including registered but normally unselected songs.
constexpr std::array<char const *,22> songs{
 nullptr,"menu","menu","shella","shellb","shellc","shella","death","win2",
 nullptr,nullptr,nullptr,"intro1","intro2",nullptr,"rubble","ice","desert","volcano","losing","echo","tau"};
}
void shell_audio::start_song(){
 if(wanted<0)return;
 if(!music)music=std::make_unique<music_audio>(resources,bank,music_config);
 if(track>=0&&record)record->event(ticks,2,track);
 music->stop();track=wanted;gain=fade_next=100;
 if(record)record->event(ticks,1,track);
 auto name{std::string{songs.at(static_cast<size_t>(track))}};
 if(resources.edition()==game_edition::expansion&&battle_set_){if(track>=3&&track<=6)name="b"+name;else if(track==8)name="victory2";}
 music->load(resources.read("shell0","res32/"+name+".hmp"),65535);
 if(record)record->event(ticks,5,65535);
 music->set_volume(100);if(music_enabled&&!suspended&&!film_playing){music->play();if(record)record->event(ticks,3,track);}
}
void shell_audio::film(bool active){
 if(film_playing==active)return;
 film_playing=active;clear_sample();if(music){if(active)music->stop();else if(music_enabled&&!suspended)music->play();}
}
void shell_audio::film_sample(std::string_view name){
 auto path{resources.external("flic/"+std::string{name})};if(!path)throw std::runtime_error{"film audio missing: "+std::string{name}};
 bank.stop(0);auto data{read_file(*path)};bank.bind(0,data);bank.set_volume(0,100);play();
}
void shell_audio::song(int id){
 if(id<0||static_cast<size_t>(id)>=songs.size()||!songs[static_cast<size_t>(id)])throw std::runtime_error{"invalid shell music ID"};
 if(wanted==id)return;
 wanted=id;
 if(track==-1)start_song();
 else if(track==wanted){gain=fade_next=100;music->set_volume(100);}
 else fade_next=gain;
}
void shell_audio::region(unsigned id){
 constexpr std::array<int,7> by_region{1,1,3,4,3,3,1};
 if(id>=1000&&id<=7000)song(by_region.at(id/1000-1));
}
void shell_audio::enabled(bool song_enabled,bool sample_enabled){
 if(music_enabled!=song_enabled){
  music_enabled=song_enabled;
  if(music){if(music_enabled&&!suspended&&!film_playing)music->play();else music->stop();}
 }
 effects_enabled=sample_enabled;
}
bool shell_audio::load(std::string_view name){
 clear_sample();auto path{normalise_path(name)};if(path.find('/')==std::string::npos)path="res32/"+path;
 bool found{};for(auto const &[archive_name,volume]:resources.archives())if((archive_name=="shell0"||(resources.edition()==game_edition::expansion&&(volume.usage_mask()&256)))&&volume.find(path))found=true;
 if(!found)return false;
 auto data{resources.read("shell0",path)};
 bank.bind(0,data);return true;
}
void shell_audio::volume(uint8_t value){bank.set_volume(0,static_cast<int8_t>(value));}
void shell_audio::play(){if(effects_enabled&&!suspended)bank.play(0);}
void shell_audio::clear_sample(){bank.stop(0);}
void shell_audio::speak(std::string_view name,bool enabled){
 stop_speech();if(!enabled||suspended)return;
 auto path{normalise_path(name)};if(path.find('/')==std::string::npos)path="snc/"+path;
 bank.bind(1,resources.read("speech",path));bank.set_volume(1,100);bank.play(1);
}
void shell_audio::stop_speech(){bank.stop(1);}
void shell_audio::door(bool closing){
 bank.stop(2);if(!effects_enabled||suspended)return;
 bank.bind(2,resources.read("shell0",closing?"res32/menuclsa.raw":"res32/menuopn2.raw"));bank.set_volume(2,100);bank.play(2);
}
void shell_audio::suspend(){
 suspended=true;if(music)music->stop();clear_sample();stop_speech();bank.stop(2);output.clear();
}
void shell_audio::resume(){if(!suspended)return;suspended=false;if(wanted>=0)start_song();}
void shell_audio::advance(unsigned timer_ticks){
 for(unsigned i{};i<timer_ticks;++i){
  // GO 0x209e0 writes gain 100..1, waiting one period-two clock tick each.
  if(ticks%2==0&&track!=wanted&&music){
   if(fade_next>0){gain=fade_next--;music->set_volume(static_cast<uint8_t>(gain));if(record)record->event(ticks,4,gain);}
   else start_song();
  }
  if(music)music->tick();
  auto const next{++ticks*11025/120};
  auto data{mixer.advance(static_cast<uint16_t>((next-pcm_frames)*2))};
  auto const next_output{ticks*44100/120};auto frames{static_cast<size_t>(next_output-output_frames)};
  auto fm{music?music->render(frames):std::vector<int16_t>(frames*2)};
  for(size_t f{};f<frames;++f){
   auto const absolute{output_frames+f};
   if(absolute%4==0&&absolute>=4){
    auto const index{absolute/4-1};
    if(index>=pcm_frames&&index<next)held_pcm={data[static_cast<size_t>(index-pcm_frames)*2],data[static_cast<size_t>(index-pcm_frames)*2+1]};
   }
   for(size_t c{};c<2;++c)output.push_back(static_cast<int16_t>((static_cast<int32_t>(fm[f*2+c])+(static_cast<int32_t>(held_pcm[c])-128)*256)/2));
  }
  if(!data.empty())held_pcm={data[data.size()-2],data.back()};
  pcm_frames=next;output_frames=next_output;
 }
}
std::vector<int16_t> shell_audio::take_output(){return std::exchange(output,{});}
}
