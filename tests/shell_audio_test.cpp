#include "audio/shell_audio.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
struct event {
 uint64_t tick;uint8_t kind;int value;
 bool operator==(event const &)const=default;
};
struct recording: shell_audio::observer {
 std::vector<::event> events;
 void event(uint64_t tick,uint8_t kind,int value)override{events.push_back({tick,kind,value});}
};
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESSHMUS1")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned total{};
 for(unsigned c{};c<cases;++c){
  auto existing{f.integer<uint8_t>()},enabled{f.integer<uint8_t>()},old{f.integer<uint8_t>()},next{f.integer<uint8_t>()};
  auto count{f.integer<uint16_t>()};std::vector<event> expected;
  for(unsigned i{};i<count;++i)expected.push_back({f.integer<uint32_t>(),f.integer<uint8_t>(),f.integer<uint16_t>()});
  shell_audio audio{r};audio.enabled(enabled!=0,true);if(existing)audio.song(old);
  recording log;audio.record=&log;audio.song(next);audio.advance(202);
  if(log.events!=expected){
   std::cerr<<"shell music case "<<c<<" actual "<<log.events.size()<<" expected "<<expected.size()<<'\n';
   for(size_t i{};i<std::min(log.events.size(),expected.size());++i)if(log.events[i]!=expected[i]){
    auto const &a{log.events[i]},&b{expected[i]};std::cerr<<i<<": "<<a.tick<<','<<int(a.kind)<<','<<a.value<<" vs "<<b.tick<<','<<int(b.kind)<<','<<b.value<<'\n';break;
   }
   throw std::runtime_error{"original shell music dispatch mismatch"};
  }
  if(audio.song_id()!=next)throw std::runtime_error{"requested song did not become current"};
  total+=count;
 }
 f.require_end();
 shell_audio sound{r};sound.enabled(false,true);
 for(auto name:{"HAMMR1R.RAW","HAMMR2R.RAW","HAMMR3R.RAW","VEHIC1R.RAW","VEHIC2R.RAW","RATCHT1R.RAW"}){
  if(!sound.load(name))throw std::runtime_error{"missing workshop sound"};
  sound.volume(24);sound.play();sound.advance(120);auto pcm{sound.take_output()};
  if(pcm.size()!=88200||std::ranges::none_of(pcm,[](auto v){return v!=0;}))throw std::runtime_error{"workshop sound did not reach stereo output"};
 }
 sound.enabled(false,false);if(sound.effects()!=nullptr)throw std::runtime_error{"disabled sound script remains enabled"};
 if(sound.load("absent.raw"))throw std::runtime_error{"missing sample falsely available"};
 shell_audio music{r};music.region(1000);music.advance(240);auto pcm{music.take_output()};
 if(pcm.size()!=176400||std::ranges::none_of(pcm,[](auto v){return v!=0;}))throw std::runtime_error{"menu music did not reach stereo output"};
 std::cout<<cases<<" original music transitions / "<<total<<" events agree; menu music and all six workshop sounds reach stereo output\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
