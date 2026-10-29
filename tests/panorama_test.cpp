#include "render/panorama.h"
#include "resources/resource_store.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESPANOR1")f.fail("fixture version");
 auto worlds{f.integer<uint16_t>()};unsigned frames{},failures{};
 for(unsigned i{};i<worlds;++i){
  auto world{f.integer<uint16_t>()},cases{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
  std::vector<bitmap> images;
  if(!env.resource_names[0].empty())images=read_bitmaps(resources.read("simvol0","dba/"+env.resource_names[0]+".DBA"));
  for(unsigned j{};j<cases;++j){
   auto pitch{f.integer<uint16_t>()},roll{f.integer<uint16_t>()},heading{f.integer<uint16_t>()},shift{f.integer<uint16_t>()};
   background_view view;view.pitch=pitch;view.roll=roll;view.projection_shift=shift;
   view.centre={f.integer<int32_t>(),f.integer<int32_t>()};view.offset={f.integer<int32_t>(),f.integer<int32_t>()};
   view.bounds={f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
   auto crc{f.integer<uint32_t>()};auto count{f.integer<uint16_t>()};auto label{std::to_string(world)+"/"+std::to_string(j)};
   auto bands{make_background_bands(env,view)};auto placements{place_panorama(images,bands,view,heading,env.background.horizon_offset)};
   if(placements.size()!=count)throw std::runtime_error{label+" panorama command count"};
   for(auto const &p:placements){
    bool rotated{f.integer<uint8_t>()!=0};auto index{f.integer<uint16_t>()},angle{f.integer<uint16_t>()};vec2<int32_t> pos{f.integer<int32_t>(),f.integer<int32_t>()};
    if(rotated!=p.rotated||index!=p.frame||angle!=p.angle||pos!=p.position){
     std::cerr<<label<<" expected "<<index<<" "<<pos.x<<","<<pos.y<<" angle "<<angle<<" actual "<<p.frame<<" "<<p.position.x<<","<<p.position.y<<" angle "<<p.angle<<'\n';
     throw std::runtime_error{"panorama placement differs"};
    }
   }
   framebuffer frame;frame.clear(37);draw_background_bands(frame,bands,view.bounds);draw_panorama(frame,images,placements,view.bounds);++frames;
   if(crc32(frame.pixels())!=crc){
    if(failures<5){std::ofstream out{"/tmp/es-panorama-"+std::to_string(world)+"-"+std::to_string(j)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<label<<" panorama pixels differ\n";
   }
  }
 }
 f.require_end();if(failures)throw std::runtime_error{std::to_string(failures)+" panorama frames differ"};
 std::cout<<frames<<" original panorama frames and drawing placements match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
