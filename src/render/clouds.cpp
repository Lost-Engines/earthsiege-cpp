#include "clouds.h"
#include "render/texture_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace earthsiege {
cloud_field::cloud_field(world_environment const &env,std::vector<bitmap> textures)
 :images{std::move(textures)}{
 if(env.effect_parameters[0]<0||images.size()<3)throw std::runtime_error{"invalid cloud field"};
 field.resize(static_cast<size_t>(env.effect_parameters[0]));recycled.resize(field.size());
}
void cloud_field::draw(framebuffer &target,vec3<int32_t> const &position,vec3<uint16_t> const &angles,
                       camera_projection const &projection,rectangle bounds,uint8_t detail,random_state &random){
 active=static_cast<int16_t>(field.size()*(static_cast<unsigned>(detail)+1)/256);
 // Movement precedes lazy initialization, whose initial wind is zero.
 for(int i{};i<active;++i)for(unsigned axis{};axis<3;++axis)
  field[static_cast<size_t>(i)][axis]=wrap_long(static_cast<int64_t>(field[static_cast<size_t>(i)][axis])+drift[axis]);
 if(!ready){
  // Base ring offsets are generated even though the derived distance method
  // is empty (0x25a50); preserve their consumption of the shared visual RNG.
  for(unsigned i{};i<8;++i)random.next();
  previous=position;
  auto component=[](int32_t length,uint16_t angle){return wrap_long((static_cast<int64_t>(length)*model_cosine(angle)+8192)>>14);};
  for(auto &point:field){
   auto const radius{static_cast<int32_t>(static_cast<uint16_t>(random.next()))*2};
   auto const angle{static_cast<uint16_t>(random.next())};
   point={wrap_long(static_cast<int64_t>(position.x)+component(radius,angle)),
          wrap_long(static_cast<int64_t>(position.y)+component(radius,static_cast<uint16_t>(angle-16384))),10000};
  }
  auto const angle{static_cast<uint16_t>(random.next())};int32_t const speed{random.bounded(100)+50};
  drift={component(speed,angle),component(speed,static_cast<uint16_t>(angle-16384)),0};
  top_texture=random.bounded(3);ready=true;
 }
 bool const moved{position.x!=previous.x||position.y!=previous.y};
 vec2<int32_t> const middle{wrap_long(static_cast<int64_t>(previous.x)+position.x)>>1,
                           wrap_long(static_cast<int64_t>(previous.y)+position.y)>>1};
 auto camera{rotation_transform(angles)};camera.translation=position;camera=inverse_transform(camera);
 std::array<vec3<int32_t>,4> const corners{vec3<int32_t>{0,0,10000},vec3<int32_t>{25000,0,10000},vec3<int32_t>{25000,25000,10000},vec3<int32_t>{0,25000,10000}};
 for(size_t i{};i<field.size();++i){
  auto &point{field[i]};
  int32_t const distance{fast_distance_2d({wrap_long(static_cast<int64_t>(point.x)-position.x),wrap_long(static_cast<int64_t>(point.y)-position.y)})};
  if(moved){
   if(distance>210000&&!(recycled[i]&1)){
    point.x=wrap_long(static_cast<int64_t>(middle.x)*2-point.x);point.y=wrap_long(static_cast<int64_t>(middle.y)*2-point.y);recycled[i]|=1;
   }else if(distance<190000&&(recycled[i]&1))recycled[i]&=0xfe;
  }
  if(i>=static_cast<size_t>(active))continue;
  model_transform placement;placement.translation=point;
  auto const transform{compose_transforms(placement,camera)};
  bool const back{position.z>=point.z+10000};
  auto const &texture{images.at(back?static_cast<size_t>(top_texture):0)};
  int32_t const width{((static_cast<int32_t>(texture.width)-1)>>3)<<3},height{static_cast<int32_t>(texture.height)-1};
  std::array<camera_vertex,4> vertices;
  for(size_t j{};j<4;++j){
   auto v{rotate_vector(corners[j],transform)};
   for(unsigned axis{};axis<3;++axis)v[axis]=wrap_long(static_cast<int64_t>(v[axis])+transform.translation[axis]);
   vertices[j].position=v;vertices[j].texture={(j==1||j==2)?width:0,j>=2?height:0};
  }
  auto projected{project_polygon(vertices,projection,projection_source::terrain)};
  // Untranslated texture mode 5 reverses UVs along with geometry.
  if(back&&projected.size()>2)std::ranges::reverse(projected);
  draw_textured_polygon(target,projected,texture,bounds,{},target.clip()?texture_scan_direction::rows:texture_scan_direction::columns);
 }
 previous=position;
}
}
