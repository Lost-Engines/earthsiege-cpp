#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/briefing_map.h"
#include "resources/resource_store.h"
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESBTOUR2")f.fail("fixture version");
 auto cases{f.integer<uint32_t>()};unsigned total{},frames{},failures{};
 for(unsigned c{};c<cases;++c){
  auto script{read_mission_script(f.take(f.integer<uint32_t>()))};auto players{f.integer<uint16_t>()},texts{f.integer<uint16_t>()};std::vector<std::string> text;
  for(unsigned j{};j<texts;++j)text.push_back(f.cstring());
  briefing_map map{resources,script,players,text};map.start_tour();auto count{f.integer<uint32_t>()};
  for(unsigned i{};i<count;++i){
   auto now{f.integer<uint32_t>()};auto stage{f.integer<int16_t>()};auto deadline{f.integer<uint32_t>()};auto objective{f.integer<int16_t>()};
   auto visible{f.integer<uint8_t>()},route{f.integer<uint8_t>()},waypoints{f.integer<uint8_t>()},ready{f.integer<uint8_t>()};auto resume{f.integer<int16_t>()};
   vec3<int32_t> camera{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};map.update(now);auto const &s{map.state()};
   if(s.stage!=stage||s.deadline!=deadline||s.objective!=objective||s.players!=visible||s.route_points!=route||s.waypoints!=waypoints||s.ready!=bool(ready)||s.resume!=resume||s.camera!=camera){
    std::cerr<<"category "<<c<<", tick "<<now<<": stage "<<s.stage<<"/"<<stage<<", deadline "<<s.deadline<<"/"<<deadline<<", objective "<<s.objective<<"/"<<objective<<", visible "<<s.players<<"/"<<int(visible)<<", routes "<<s.route_points<<"/"<<int(route)<<", camera "<<s.camera.x<<","<<s.camera.y<<","<<s.camera.z<<"/"<<camera.x<<","<<camera.y<<","<<camera.z<<"\n";
    f.fail("original tour state differs");
   }
   if(f.integer<uint8_t>()){
    auto const expected{f.integer<uint32_t>()};framebuffer frame;frame.clear(37);map.draw(frame);++frames;
    if(crc32(frame.pixels())!=expected){
     if(!failures){std::ofstream out{"/tmp/es-briefing-tour-difference.raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
     ++failures;std::cerr<<"tour frame "<<c<<"/"<<now<<" stage "<<s.stage<<" differs\n";
    }
   }++total;
  }
 }
 f.require_end();if(failures)f.fail(std::to_string(failures)+" tour frames differ");std::cout<<frames<<" frames and "<<total<<" original briefing-tour updates match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
