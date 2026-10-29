#include "shell/cockpit_impact.h"
#include "resources/binary_reader.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESIMPCT1")f.fail("fixture version");
 auto cases{f.integer<uint16_t>()};unsigned updates{};
 for(unsigned c{};c<cases;++c){
  random_state rng{f.integer<int16_t>()};cockpit_impact owner;cockpit_impact_view view;
  view.mode=f.integer<uint8_t>();view.transitioning=f.integer<uint8_t>();view.palette_available=f.integer<uint8_t>();
  for(auto &v:view.origin)v=f.integer<int32_t>();
  auto steps{f.integer<uint16_t>()};
  for(unsigned step{};step<steps;++step){
   auto now{f.integer<uint32_t>()};auto const old_mode{view.mode};view.mode=f.integer<uint8_t>();view.transitioning=f.integer<uint8_t>();view.palette_available=f.integer<uint8_t>();
   auto action{f.integer<uint8_t>()};bool expected_result{f.integer<uint8_t>()!=0};
   if(action==2){
    view.origin=view.mode==1?std::array<int32_t,2>{0,200}:view.mode==2?std::array<int32_t,2>{320,0}:view.mode==3?std::array<int32_t,2>{-320,0}:std::array<int32_t,2>{0,0};
    owner.change_view(old_mode,view);
   }else if(action==1)owner.hit(now,view,rng);else if(owner.update(now,view,rng)!=expected_result)f.fail("original active result");
   auto const &s{owner.snapshot()};
   auto check32=[&](auto value){if(static_cast<uint32_t>(value)!=f.integer<uint32_t>())f.fail("original clock/page state at case "+std::to_string(c)+" step "+std::to_string(step));};
   auto check8=[&](auto value){if(static_cast<unsigned>(value)!=f.integer<uint8_t>())f.fail("original palette/shake flags");};
   check32(s.expiry);check32(s.next_flash);
   check8(s.flash);check8(s.impact_palette);check8(s.saved_palette);check8(s.alternate_palette);check8(s.shaking);
   for(auto v:s.saved_origin)check32(v);
   for(auto v:s.current_origin)check32(v);
   check32(s.lower);check32(s.upper);
   if(s.displacement!=f.integer<int16_t>())f.fail("original displacement");
   check32(7);check32(10-s.displacement);check32(290);check32(165-s.displacement);
   if(f.integer<int16_t>()!=20-s.displacement)f.fail("original span-list displacement");
   check32(0);check32(-s.displacement);
   for(auto word:rng.snapshot().words)if(word!=f.integer<uint16_t>())f.fail("original visual RNG");
   check8(rng.snapshot().first);check8(rng.snapshot().second);++updates;
  }
 }
 f.require_end();std::cout<<updates<<" original impact clock, palette, clip, widget and complete RNG states agree\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
