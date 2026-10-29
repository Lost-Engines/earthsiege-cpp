#include "render/shell_animation_artwork.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCINEM1")f.fail("fixture version");
 auto cases{f.integer<uint16_t>()};unsigned frames{},writes{};
 for(unsigned c{};c<cases;++c){
  auto name{f.cstring()};auto count{f.integer<uint16_t>()};
  shell_animation player{read_presentation(r.read("shell0",name+".ttm"))};
  shell_animation_artwork art{r,name.substr(0,name.find('/')+1)};random_state random{17};
  framebuffer visible,background;background.clear(37);
  for(unsigned i{};i<count;++i){
   auto elapsed{f.integer<int16_t>()};visible=background;player.advance(elapsed,random);art.apply(player.commands(),visible,background);
   auto const &s{player.state()};bool ok{f.integer<uint16_t>()==s.page};ok=(f.integer<uint32_t>()==s.mode)&&ok;
   for(bool flag:{s.wait_latched,s.fade_latched,s.executed})ok=(f.integer<uint8_t>()==flag)&&ok;
   for(int16_t value:{s.remaining,s.period,s.jump,s.branch,s.depth,s.x,s.y})ok=(f.integer<int16_t>()==value)&&ok;
   for(auto word:random.snapshot().words)ok=(f.integer<uint16_t>()==word)&&ok;
   ok=(f.integer<uint8_t>()==random.snapshot().first)&&ok;ok=(f.integer<uint8_t>()==random.snapshot().second)&&ok;
   auto const front{f.integer<uint32_t>()},back{f.integer<uint32_t>()};auto amount{f.integer<uint16_t>()};
   bool pixels{crc32(visible.pixels())==front&&crc32(background.pixels())==back};
   bool colours{amount==art.palette_frames().size()};
   for(unsigned p{};p<amount;++p){
    auto image{f.integer<uint32_t>()};auto dac{f.take(768)};
    if(p<art.palette_frames().size()){
     auto const &frame{art.palette_frames()[p]};if(image!=crc32(frame.image.pixels()))std::cerr<<"frame event "<<p<<" crc "<<image<<"/"<<crc32(frame.image.pixels())<<"\n";colours=(image==crc32(frame.image.pixels()))&&colours;
     for(size_t j{};j<768;++j)if(dac[j]!=frame.colours.dac_colours[j/3][j%3]){std::cerr<<"palette event "<<p<<" byte "<<j<<" expected "<<int(dac[j])<<" actual "<<int(frame.colours.dac_colours[j/3][j%3])<<"\n";colours=false;break;}
    }
   }
   if(!ok||!pixels||!colours){
    std::ofstream out{"build/cinematic-actual.raw",std::ios::binary};out.write(reinterpret_cast<char const *>(visible.pixels().data()),visible.pixels().size());
    std::cerr<<name<<" update "<<i<<" page "<<s.page<<" state "<<ok<<" pixels "<<pixels<<" palettes "<<colours<<" count "<<amount<<"/"<<art.palette_frames().size()<<'\n';
    f.fail("original cinematic mismatch");
   }
   ++frames;writes+=amount;
  }
 }
 f.require_end();std::cout<<frames<<" original cinematic updates and "<<writes<<" palette submissions agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
