#include "render/clouds.h"
#include "resources/cockpit.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
template<typename T> vec3<T> vector(binary_reader &f){return {f.integer<T>(),f.integer<T>(),f.integer<T>()};}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESCLOUD1")f.fail("fixture version");
 auto streams{f.integer<uint16_t>()};unsigned frames{},draws{},failures{};
 for(unsigned stream{};stream<streams;++stream){
  auto world{f.integer<uint16_t>()};auto seed{f.integer<int16_t>()};auto count{f.integer<uint16_t>()},points{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
  cloud_field clouds{env,read_bitmaps(resources.read("simvol0","dba/"+env.resource_names[1]+".DBA"))};random_state random{seed};
  for(unsigned step{};step<count;++step){
   auto position{vector<int32_t>(f)};auto angles{vector<uint16_t>(f)};auto detail{f.integer<uint8_t>()};auto repeat{f.integer<uint16_t>()};
   auto mask{f.cstring()};auto window{f.integer<uint8_t>()};
   camera_projection projection;projection.perspective_shift=f.integer<uint16_t>();projection.near_distance=1<<projection.perspective_shift;projection.centre={f.integer<int32_t>(),f.integer<int32_t>()};
   rectangle bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
   bool ready{f.integer<uint8_t>()!=0};auto active{f.integer<int16_t>()};auto anchor{vector<int32_t>(f)},wind{vector<int32_t>(f)};auto texture{f.integer<int32_t>()};auto crc{f.integer<uint32_t>()};
   random_state::state expected;for(auto &word:expected.words)word=f.integer<uint16_t>();expected.first=f.integer<uint8_t>();expected.second=f.integer<uint8_t>();
   framebuffer frame;std::optional<cockpit_artwork> cockpit;
   if(!mask.empty()){cockpit.emplace(read_cockpit(resources,mask));frame.clip(&cockpit->windows.at(window).mask);}
   for(unsigned draw{};draw<repeat;++draw){frame.clear(37);clouds.draw(frame,position,angles,projection,bounds,detail,random);++draws;}++frames;
   auto const label{std::to_string(seed)+"/"+std::to_string(step)};
   if(clouds.initialized()!=ready||clouds.active_count()!=active||clouds.anchor()!=anchor||clouds.wind()!=wind||clouds.back_texture()!=texture||clouds.points().size()!=points)f.fail(label+" cloud state");
   for(auto const &actual:clouds.points()){
    auto point{vector<int32_t>(f)};
    if(actual!=point){std::cerr<<label<<" point "<<actual<<" expected "<<point<<'\n';f.fail("cloud position");}
   }
   for(auto flag:clouds.flags())if(flag!=f.integer<uint8_t>())f.fail(label+" recycling flag");
   auto const &actual{random.snapshot()};
   if(actual.words!=expected.words||actual.first!=expected.first||actual.second!=expected.second)f.fail(label+" visual RNG");
   if(crc32(frame.pixels())!=crc){
    if(failures<12){std::ofstream out{"/tmp/es-clouds-"+std::to_string(seed)+"-"+std::to_string(step)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<label<<" cloud pixels differ\n";
   }
  }
 }
 f.require_end();if(failures)throw std::runtime_error{std::to_string(failures)+" cloud frames differ"};
 std::cout<<frames<<" complete cloud frames / "<<draws<<" draws, fields and RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
