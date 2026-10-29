#include "shell/bay_scene.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};armament_catalog catalog{r};hangar_artwork artwork{r};
 auto prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBAYPX2")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned frames{};
 for(unsigned c{};c<cases;++c){
  auto slot{f.integer<uint16_t>()},remaining{f.integer<uint16_t>()},count{f.integer<uint16_t>()};
  auto roster{read_hangar(r.read("shell0","car/sm_init.dat"))};career_state crew;crew.selected_crew=0;
  for(size_t i{};i<4;++i)crew.crew[i].hangar_slot=static_cast<int16_t>(i);
  random_state random{17};framebuffer base;base.clear(37);bay_scene scene{r};
  scene.reset(roster,crew,true,0,prices,random,base);roster.selected=static_cast<int16_t>(slot);
  if(remaining){
   roster.construction_slot=roster.selected;roster.construction_remaining=static_cast<int16_t>(remaining);
   std::ranges::fill(roster.find(roster.selected)->weapons,int16_t{17});scene.construction(roster,prices,random);
  }
  scene.redraw(roster,artwork,catalog,-1,base,random);
  for(unsigned step{};step<count;++step){
   auto elapsed{f.integer<int16_t>()};if(step)scene.advance(elapsed,random);
   auto visible{f.integer<uint32_t>()},background{f.integer<uint32_t>()};
   bool const pixels{crc32(scene.image().pixels())==visible&&crc32(scene.retained().pixels())==background};
   bool rng{true};for(auto w:random.snapshot().words)rng=(f.integer<uint16_t>()==w)&&rng;
   rng=(f.integer<uint8_t>()==random.snapshot().first)&&rng;rng=(f.integer<uint8_t>()==random.snapshot().second)&&rng;
   bool states{true};
   for(size_t i{};i<25;++i){
    auto const &s{scene.state(i)};bool ok{f.integer<uint16_t>()==s.page};ok=(f.integer<uint32_t>()==s.mode)&&ok;
    for(bool v:{s.wait_latched,s.fade_latched,s.executed})ok=(f.integer<uint8_t>()==v)&&ok;
    for(auto v:{s.remaining,s.period,s.jump,s.branch,s.depth,s.x,s.y})ok=(f.integer<int16_t>()==v)&&ok;
    if(!ok){std::cerr<<"program "<<i<<" differs at "<<slot<<":"<<step<<'\n';states=false;}
   }
   if(!pixels||!rng||!states){
    std::cerr<<"bay "<<slot<<" step "<<step<<" pixels "<<pixels<<" random "<<rng<<" states "<<states
      <<" visible "<<crc32(scene.image().pixels())<<"/"<<visible<<" retained "<<crc32(scene.retained().pixels())<<"/"<<background<<'\n';
    throw std::runtime_error{"original complete bay mismatch"};
   }++frames;
  }
 }
 f.require_end();std::cout<<frames<<" original complete bay frames and shared animation states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
