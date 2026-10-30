#include "render/model_renderer.h"
#include "simulation/model_math.h"
#include <iostream>
#include <format>
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=2)return 1;
 auto const data{read_file(argv[1])};binary_reader f{data};auto const magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBOUND1")f.fail("fixture version");
 auto const count{f.integer<uint32_t>()};
 for(uint32_t i{};i<count;++i){
  part_header h;h.radius=f.integer<int16_t>();
  for(unsigned a{};a<3;++a)h.centre[a]=f.integer<int16_t>();
  vec3<uint16_t> angles;for(unsigned a{};a<3;++a)angles[a]=f.integer<uint16_t>();
  auto transform{rotation_transform(angles)};
  for(unsigned a{};a<3;++a)transform.translation[a]=f.integer<int32_t>();
  camera_projection p;p.perspective_shift=static_cast<unsigned>(f.integer<int32_t>());p.near_distance=f.integer<int32_t>();
  p.centre={f.integer<int32_t>(),f.integer<int32_t>()};
  rectangle bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
  bool const clip{f.integer<uint8_t>()!=0};auto const expected{f.integer<uint8_t>()};
  auto const actual{classify_model_bounds(h,transform,p,bounds,clip)};
  if(actual!=expected)throw std::runtime_error{std::format("case {}: bounds classification {} expected {}",i,actual,expected)};
 }
 f.require_end();std::cout<<count<<" original whole-object bounds decisions agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
