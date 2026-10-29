#include "shell/shell_animation.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
struct sound_sink: shell_animation_sound {
 bool available{true},loaded{};
 std::vector<std::pair<int,std::string>> events;
 bool load(std::string_view name)override{
  if(loaded)events.emplace_back(2,"");
  events.emplace_back(1,"res32/"+normalise_path(name));
  return loaded=available;
 }
 void play()override{events.emplace_back(3,"");}
 void volume(uint8_t value)override{events.emplace_back(4,std::to_string(value));}
};
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESHSND01")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned total{};
 for(unsigned p{};p<cases;++p){
  auto repeat{f.integer<uint8_t>()};auto count{f.integer<uint32_t>()};
  shell_animation player{read_presentation(r.read("shell0","res32/sounds.ttm")),repeat!=0};random_state random{17};sound_sink sound;
  for(unsigned step{};step<count;++step){
   auto elapsed{f.integer<int16_t>()},page{f.integer<int16_t>()};auto enabled{f.integer<uint8_t>()};sound.available=f.integer<uint8_t>()!=0;
   sound.events.clear();if(page>=0)player.select_page(static_cast<uint16_t>(page));player.advance(elapsed,random,enabled?&sound:nullptr);
   auto const &s{player.state()};bool ok{f.integer<uint16_t>()==s.page};ok=(f.integer<uint32_t>()==s.mode)&&ok;
   for(bool flag:{s.wait_latched,s.fade_latched,s.executed})ok=(f.integer<uint8_t>()==flag)&&ok;
   for(int16_t value:{s.remaining,s.period,s.jump,s.branch,s.depth,s.x,s.y})ok=(f.integer<int16_t>()==value)&&ok;
   for(auto word:random.snapshot().words)ok=(f.integer<uint16_t>()==word)&&ok;
   ok=(f.integer<uint8_t>()==random.snapshot().first)&&ok;ok=(f.integer<uint8_t>()==random.snapshot().second)&&ok;
   auto count_events{f.integer<uint16_t>()};ok=(count_events==sound.events.size())&&ok;
   for(unsigned i{};i<count_events;++i){
    auto kind{f.integer<uint8_t>()};auto text{f.cstring()};
    if(i>=sound.events.size()||sound.events[i]!=std::pair<int,std::string>{kind,text}){
     ok=false;std::cerr<<"event "<<int(kind)<<' '<<text<<" differs\n";
    }
   }
   if(!ok){std::cerr<<"case "<<p<<" step "<<step<<'\n';throw std::runtime_error{"original shell animation sound mismatch"};}
   ++total;
  }
 }
 f.require_end();std::cout<<total<<" original sound-script updates agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
