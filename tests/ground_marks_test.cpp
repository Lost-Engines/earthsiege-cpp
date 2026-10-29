#include "render/ground_marks.h"
#include "resources/cockpit.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
template<typename T> vec3<T> vector(binary_reader &f){return {f.integer<T>(),f.integer<T>(),f.integer<T>()};}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESGRND01")f.fail("fixture version");
 auto streams{f.integer<uint16_t>()};unsigned frames{},failures{};
 for(unsigned stream{};stream<streams;++stream){
  auto world{f.integer<uint16_t>()};auto seed{f.integer<int16_t>()};auto count{f.integer<uint16_t>()},points{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
  ground_marks marks{env};random_state random{seed};
  for(unsigned step{};step<count;++step){
   auto position{vector<int32_t>(f)};auto angles{vector<uint16_t>(f)};auto detail{f.integer<uint8_t>()};
   auto mask{f.cstring()};auto window{f.integer<uint8_t>()};
   camera_projection projection;projection.perspective_shift=f.integer<uint16_t>();projection.centre={f.integer<int32_t>(),f.integer<int32_t>()};
   rectangle bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
   bool ready{f.integer<uint8_t>()!=0};auto active{f.integer<int16_t>()};auto anchor{vector<int32_t>(f)};auto crc{f.integer<uint32_t>()};
   random_state::state expected;for(auto &word:expected.words)word=f.integer<uint16_t>();expected.first=f.integer<uint8_t>();expected.second=f.integer<uint8_t>();
   framebuffer frame;frame.clear(37);
   std::optional<cockpit_artwork> cockpit;
   if(!mask.empty()){cockpit.emplace(read_cockpit(resources,mask));frame.clip(&cockpit->windows.at(window).mask);}
   marks.draw(frame,position,angles,projection,bounds,detail,random);++frames;
   auto const label{std::to_string(world)+"/"+std::to_string(seed)+"/"+std::to_string(step)};
   if(marks.initialized()!=ready||marks.active_count()!=active||marks.anchor()!=anchor||marks.points().size()!=points)f.fail(label+" field state");
   for(auto const &actual:marks.points()){
    auto point{vector<int32_t>(f)};
    if(actual!=point){std::cerr<<label<<" point "<<actual<<" expected "<<point<<'\n';f.fail("mark position");}
   }
   auto const &actual{random.snapshot()};
   if(actual.words!=expected.words||actual.first!=expected.first||actual.second!=expected.second)f.fail(label+" visual RNG");
   if(crc32(frame.pixels())!=crc){
    if(failures<8){std::ofstream out{"/tmp/es-ground-marks-"+std::to_string(world)+"-"+std::to_string(seed)+"-"+std::to_string(step)+".raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}
    ++failures;std::cerr<<label<<" ground pixels differ\n";
   }
  }
 }
 f.require_end();if(failures)throw std::runtime_error{std::to_string(failures)+" frames differ"};
 std::cout<<frames<<" complete ground-mark frames, fields and RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
