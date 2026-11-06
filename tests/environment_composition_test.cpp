#include "render/environment_renderer.h"
#include "resources/cockpit.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
template<typename T> vec3<T> vector(binary_reader &f){return {f.integer<T>(),f.integer<T>(),f.integer<T>()};}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESENV001")f.fail("fixture version");
 auto worlds{f.integer<uint16_t>()};unsigned frames{},failures{};
 for(unsigned stream{};stream<worlds;++stream){
  auto world{f.integer<uint16_t>()};auto seed{f.integer<int16_t>()};auto count{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"),resources.edition())};
  auto colours{read_palette(resources.read("simvol0","dpl/WORLD"+std::to_string(world&~1U)+".DPL"))};
  environment_renderer renderer{resources,env};random_state random{seed};
  for(unsigned step{};step<count;++step){
   auto position{vector<int32_t>(f)};auto angles{vector<uint16_t>(f)};environment_detail detail;
   detail.panorama=f.integer<uint8_t>()!=0;detail.clouds=f.integer<uint8_t>();detail.ground=f.integer<uint8_t>();
   auto mask{f.cstring()};auto window{f.integer<uint8_t>()};
   camera_projection projection;projection.perspective_shift=f.integer<uint16_t>();projection.near_distance=1<<projection.perspective_shift;projection.centre={f.integer<int32_t>(),f.integer<int32_t>()};
   rectangle bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};vec2<int32_t> offset{f.integer<int32_t>(),f.integer<int32_t>()};
   auto crc{f.integer<uint32_t>()};random_state::state expected;for(auto &w:expected.words)w=f.integer<uint16_t>();expected.first=f.integer<uint8_t>();expected.second=f.integer<uint8_t>();
   framebuffer frame;frame.clear(37);std::optional<cockpit_artwork> cockpit;
   if(!mask.empty()){cockpit.emplace(read_cockpit(resources,mask));frame.clip(&cockpit->windows.at(window).mask);}
   renderer.draw(frame,position,angles,projection,bounds,random,colours,detail,offset);++frames;
   auto const label{std::to_string(world)+"/"+std::to_string(step)};auto const &actual{random.snapshot()};
   if(actual.words!=expected.words||actual.first!=expected.first||actual.second!=expected.second)f.fail(label+" composed visual RNG");
   for(size_t index{64};index<80;++index)for(unsigned component{};component<3;++component)
    if(colours.dac_colours[index][component]!=f.integer<uint8_t>())f.fail(label+" reflection palette");
   if(crc32(frame.pixels())!=crc){
    if(failures<6){std::ofstream out{"/tmp/es-environment-"+std::to_string(world)+"-"+std::to_string(step)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<label<<" composed pixels differ\n";
   }
  }
 }
 f.require_end();if(failures)throw std::runtime_error{std::to_string(failures)+" environment frames differ"};
 std::cout<<frames<<" complete original environment frames, reflection palettes and shared RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
