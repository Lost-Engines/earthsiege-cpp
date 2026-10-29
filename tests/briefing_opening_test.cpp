#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/briefing_map.h"
#include "resources/resource_store.h"
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESBROPEN")f.fail("fixture version");
 auto cases{f.integer<uint32_t>()};unsigned frames{},failures{};
 for(unsigned c{};c<cases;++c){
  auto script{read_mission_script(f.take(f.integer<uint32_t>()))};auto players{f.integer<uint16_t>()},texts{f.integer<uint16_t>()};std::vector<std::string> text;
  for(unsigned j{};j<texts;++j)text.push_back(f.cstring());
  briefing_map map{resources,script,players,text};map.start_tour(true);
  auto deadline{f.integer<uint32_t>()},count{f.integer<uint32_t>()};
  for(unsigned i{};i<count;++i){
   auto tick{f.integer<uint32_t>()},crc{f.integer<uint32_t>()};map.update(tick);
   framebuffer frame;frame.clear(37);map.draw(frame);++frames;
   if(crc32(frame.pixels())!=crc){
    if(!failures){std::ofstream out{"/tmp/es-briefing-opening-difference.raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<"category "<<c<<" opening "<<tick<<" differs\n";
   }
  }
  if(map.state().deadline!=deadline)f.fail("opening delay differs");
 }
 f.require_end();if(failures)f.fail(std::to_string(failures)+" opening frames differ");
 std::cout<<frames<<" original opening frames and deadlines match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
