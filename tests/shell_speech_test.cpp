#include "resources/shell_speech.h"
#include "resources/presentation.h"
#include "resources/resource_store.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESSPSNC1")f.fail("fixture version");
 auto groups{f.integer<uint16_t>()};unsigned states{},clips{};
 for(unsigned g{};g<groups;++g){
  auto name{f.cstring()};auto count{f.integer<uint16_t>()};
  auto group{read_shell_speech_group(resources.read("shell0",name))};
  if(group.clips.size()!=count)f.fail("original group clip count");
  auto program{read_presentation(resources.read("shell0","res32/"+group.program))};
  for(size_t i{};i<count;++i){
   auto actual_program{f.cstring()},sync{f.cstring()},sample{f.cstring()};
   if(group.program!=actual_program||group.clips[i].sync!=sync||group.clips[i].sample!=sample)f.fail("original speech selection");
   auto amount{f.integer<uint16_t>()},ticks{f.integer<uint16_t>()};
   auto frames{read_shell_mouth_timeline(resources.read("shell0","snc/"+sync))};
   if(frames.size()!=amount)f.fail("original timeline size");
   for(auto const &frame:frames){
    auto page{f.integer<uint16_t>()},deadline{f.integer<uint16_t>()};
    if(frame.page!=page||frame.deadline!=deadline)f.fail("original cumulative timeline");
   }
   shell_mouth mouth{frames,static_cast<uint16_t>(program.pages.size())};
   for(unsigned t{};t<ticks;++t){
    auto clock{f.integer<uint16_t>()};auto page{f.integer<int16_t>()};auto cursor{f.integer<uint16_t>()};
    auto active{f.integer<uint8_t>()},pending{f.integer<uint8_t>()};mouth.update(clock);
    if(mouth.page()!=page||mouth.cursor()!=cursor||mouth.active()!=static_cast<bool>(active)||mouth.pending_stop()!=static_cast<bool>(pending)){
     throw std::runtime_error{sync+" at "+std::to_string(clock)+" differs from original shell mouth"};}
    ++states;
   }++clips;
  }
 }
 f.require_end();std::cout<<clips<<" original shell clips / "<<states<<" mouth states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
