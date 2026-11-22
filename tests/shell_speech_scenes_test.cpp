#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/shell_speech_scene.h"
using earthsiege::binary_reader;
using earthsiege::crc32;
using earthsiege::framebuffer;
using earthsiege::random_state;
using earthsiege::read_file;
using earthsiege::resource_store;
using earthsiege::shell_speech_scene;
using earthsiege::speech_selection;
int main(int argc,char **argv)try{
 if(argc != 3)return EXIT_FAILURE;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end()) != "ESSPSCN1")f.fail("fixture version");
 auto cases{f.integer<uint16_t>()};unsigned frames{};
 for(unsigned c{};c < cases;++c){
  speech_selection selection;selection.list_file = f.cstring();selection.text_file = f.cstring();selection.entry = f.integer<uint16_t>();selection.region = f.integer<uint16_t>();
  auto count{f.integer<uint16_t>()};framebuffer base;base.clear(37);random_state random{17};
  shell_speech_scene scene{r,selection,base,random};uint16_t time{};
  for(unsigned i{};i < count;++i){
   auto tick{f.integer<uint16_t>()};auto crc{f.integer<uint32_t>()},back{f.integer<uint32_t>()};
   while(time < tick){scene.advance(random);++time;}
   framebuffer target;scene.draw(target,2);
   bool okay{crc32(target.pixels()) == crc && crc32(scene.retained().pixels()) == back};
   for(auto word:random.snapshot().words)okay = (f.integer<uint16_t>() == word) && okay;
   okay = (f.integer<uint8_t>() == random.snapshot().first) && okay;okay = (f.integer<uint8_t>() == random.snapshot().second) && okay;
   if(!okay){
    std::ofstream dump{"build/speech-scene-actual.raw",std::ios::binary};auto const &p{target.pixels()};dump.write(reinterpret_cast<char const *>(p.data()),p.size());
    f.fail("speech scene " + std::to_string(c) + "/" + std::to_string(tick));
   }++frames;
  }
 }
 f.require_end();std::cout << frames << " complete original portrait/caption frames and RNG states agree\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr << e.what() << '\n';return EXIT_FAILURE;}
