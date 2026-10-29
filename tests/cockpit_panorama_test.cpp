#include "render/panorama.h"
#include "resources/cockpit.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESPCOCK1")f.fail("fixture version");
 auto world{f.integer<uint16_t>()},count{f.integer<uint16_t>()};unsigned failed{};
 auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
 auto images{read_bitmaps(resources.read("simvol0","dba/"+env.resource_names[0]+".DBA"))};
 for(unsigned i{};i<count;++i){
  auto name{f.cstring()};auto cockpit{read_cockpit(resources,name)};auto index{f.integer<uint8_t>()};auto const &window{cockpit.windows.at(index)};
  background_view view;view.pitch=f.integer<uint16_t>();view.roll=f.integer<uint16_t>();auto heading{f.integer<uint16_t>()};auto crc{f.integer<uint32_t>()};
  auto centre{window.projection_centre()};view.centre={centre[0],centre[1]};view.bounds=window.bounds;
  framebuffer frame;frame.clear(37);scoped_framebuffer_clip clip{frame,&window.mask};
  auto bands{make_background_bands(env,view)};draw_background_bands(frame,bands,view.bounds);
  draw_panorama(frame,images,place_panorama(images,bands,view,heading,env.background.horizon_offset),view.bounds);
  if(crc32(frame.pixels())!=crc){
   if(failed<5){std::ofstream out{"/tmp/es-cockpit-panorama-"+std::to_string(i)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
   ++failed;std::cerr<<i<<" "<<name<<" view "<<int(index)<<" differs\n";
  }
 }
 f.require_end();if(failed)throw std::runtime_error{std::to_string(failed)+" cockpit panorama frames differ"};
 std::cout<<count<<" complete original cockpit panorama frames match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
