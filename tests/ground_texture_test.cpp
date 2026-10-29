#include "render/ground_texture.h"
#include "resources/cockpit.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
template<typename T> vec3<T> vector(binary_reader &f){return {f.integer<T>(),f.integer<T>(),f.integer<T>()};}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESGTEX01")f.fail("fixture version");
 auto worlds{f.integer<uint16_t>()};unsigned frames{},failures{},scratch_failures{};
 for(unsigned stream{};stream<worlds;++stream){
  auto world{f.integer<uint16_t>()},count{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
  ground_texture ground{env,resources.read("simvol0","dba/"+env.resource_names[3]+".64K"),resources.read("simvol0","dat/"+env.resource_names[4]+".DAT")};
  for(unsigned step{};step<count;++step){
   auto position{vector<int32_t>(f)};auto angles{vector<uint16_t>(f)};auto mask{f.cstring()};auto window{f.integer<uint8_t>()};
   background_view view;view.pitch=angles.x;view.roll=angles.y;view.projection_shift=f.integer<uint16_t>();view.centre={f.integer<int32_t>(),f.integer<int32_t>()};
   view.bounds={f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};view.offset={f.integer<int32_t>(),f.integer<int32_t>()};
   auto crc{f.integer<uint32_t>()};auto commands{f.integer<uint8_t>()};
   framebuffer frame;frame.clear(37);std::optional<cockpit_artwork> cockpit;
   if(!mask.empty()){cockpit.emplace(read_cockpit(resources,mask));frame.clip(&cockpit->windows.at(window).mask);}
   auto bands{make_background_bands(env,view,false)};draw_background_bands(frame,bands,view.bounds);ground.draw(frame,position,angles,view,bands);++frames;
   auto const label{std::to_string(world)+"/"+std::to_string(step)};
   if(commands!=unsigned(ground.placement().has_value()))f.fail(label+" ground draw count");
   if(commands){
    bool rotated{f.integer<uint8_t>()!=0};auto angle{f.integer<uint16_t>()};vec2<int32_t> p{f.integer<int32_t>(),f.integer<int32_t>()};
    auto width{f.integer<uint16_t>()},height{f.integer<uint16_t>()};auto bitmap_crc{f.integer<uint32_t>()};
    auto const &actual{*ground.placement()};
    if(rotated!=actual.rotated||angle!=actual.angle||p!=actual.position||width!=ground.image().width||height!=ground.image().height){
     std::cerr<<label<<" expected "<<width<<"x"<<height<<" at "<<p<<" actual "<<ground.image().width<<"x"<<ground.image().height<<" at "<<actual.position<<'\n';f.fail("ground bitmap placement");
    }
    if(crc32(ground.image().pixels)!=bitmap_crc){
     if(scratch_failures<6){std::ofstream out{"/tmp/es-ground-texture-"+std::to_string(world)+"-"+std::to_string(step)+"-texture.raw",std::ios::binary};out.write(reinterpret_cast<char const*>(ground.image().pixels.data()),ground.image().pixels.size());}
     ++scratch_failures;std::cerr<<label<<" scratch pixels differ\n";
    }
   }
   if(crc32(frame.pixels())!=crc){
    if(failures<6){std::ofstream out{"/tmp/es-ground-texture-"+std::to_string(world)+"-"+std::to_string(step)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<label<<" ground frame differs\n";
   }
  }
 }
 f.require_end();if(failures||scratch_failures)throw std::runtime_error{std::to_string(failures)+" frames / "+std::to_string(scratch_failures)+" scratch bitmaps differ"};
 std::cout<<frames<<" original ground-texture frames and scratch bitmaps agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
