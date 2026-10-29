#include "shell/bay_scene.h"
#include "render/shell_speech_scene.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};armament_catalog catalog{r};hangar_artwork art{r};
 auto prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBAYSP1")f.fail("fixture version");
 auto cases{f.integer<uint16_t>()};unsigned frames{};
 for(unsigned c{};c<cases;++c){
  auto selected{f.integer<int16_t>()};auto group{f.integer<uint16_t>()},index{f.integer<uint16_t>()},count{f.integer<uint16_t>()};
  auto roster{read_hangar(r.read("shell0","car/sm_init.dat"))};career_state crew;crew.selected_crew=0;
  for(size_t i{};i<4;++i)crew.crew[i].hangar_slot=static_cast<int16_t>(i);
  framebuffer base;base.clear(37);random_state random{17};bay_scene bay{r};
  bay.reset(roster,crew,true,0,prices,random,base);roster.selected=selected;
  bay.redraw(roster,art,catalog,-1,base,random);bay.speech_mode(true);
  speech_selection selection;selection.list_file="snc/t"+std::to_string(group-8)+"_files.dat";selection.text_file="eng/t1_text.bin";selection.entry=index;selection.region=4000;
  shell_speech_scene speech{r,selection,bay.image(),random,&bay.retained(),
   [&](framebuffer &front,framebuffer &back,int16_t elapsed,random_state &rng){bay.compose(front,back,elapsed,rng);}};
  uint16_t time{};bool stopped{};
  for(unsigned i{};i<count;++i){
   auto tick{f.integer<uint16_t>()};auto phase{f.integer<uint8_t>()};auto crc{f.integer<uint32_t>()},back{f.integer<uint32_t>()};
   framebuffer target;uint32_t background{};
   if(!phase){while(time<tick){speech.advance(random);++time;}speech.draw(target,2);background=crc32(speech.retained().pixels());}
   else{
    if(!stopped){speech.stop(random);bay.retain(speech.image(),speech.retained());bay.speech_mode(false);stopped=true;time=0;}
    while(time<tick){bay.advance(1,random);++time;}target=bay.image();background=crc32(bay.retained().pixels());
   }
   bool pixels{crc32(target.pixels())==crc&&background==back};bool rng{true};
   for(auto word:random.snapshot().words)rng=(f.integer<uint16_t>()==word)&&rng;
   rng=(f.integer<uint8_t>()==random.snapshot().first)&&rng;rng=(f.integer<uint8_t>()==random.snapshot().second)&&rng;
   if(!pixels||!rng){
    std::ofstream dump{"build/bay-speech-actual.raw",std::ios::binary};auto const &p{target.pixels()};dump.write(reinterpret_cast<char const *>(p.data()),p.size());
    std::cerr<<"case "<<c<<" phase "<<int(phase)<<" tick "<<tick<<" pixels "<<pixels<<" random "<<rng<<" crc "<<crc32(target.pixels())<<"/"<<crc<<" back "<<background<<"/"<<back<<"\n";
    f.fail("original combined bay/portrait mismatch");
   }++frames;
  }
 }
 f.require_end();std::cout<<frames<<" original combined bay/portrait frames, cleanup and RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
