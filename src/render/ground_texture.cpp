#include "ground_texture.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <stdexcept>
namespace earthsiege {
namespace {
int32_t divide(int32_t a,int32_t b){
 if(!b||(a==std::numeric_limits<int32_t>::min()&&b==-1))throw std::domain_error{"original ground projection division outside range"};
 return a/b;
}
vec2<int32_t> rotate(vec2<int32_t> const &p,uint16_t angle,bool inverse=false){
 int32_t const c{model_cosine(angle)};
 int32_t const sine{model_cosine(static_cast<uint16_t>(angle-16384))};
 int32_t const s{inverse?-sine:sine};
 return {wrap_long((static_cast<int64_t>(p.x)*c-static_cast<int64_t>(p.y)*s+8192)>>14),
         wrap_long((static_cast<int64_t>(p.x)*s+static_cast<int64_t>(p.y)*c+8192)>>14)};
}
}
ground_texture::ground_texture(world_environment const &env,bytes texture,bytes distances)
 :source{texture.begin(),texture.end()},fog_rows{env.background.horizon_band_width>>1}{
 if(source.size()!=65536)throw std::runtime_error{"ground texture must be 256 by 256"};
 binary_reader f{distances};auto count{f.integer<uint32_t>()};
 for(uint32_t i{};i<count;++i)thresholds.push_back(f.integer<int32_t>());
 f.require_end();if(thresholds.size()<14)throw std::runtime_error{"ground distance table is too short"};
 int const base{env.background.ground_palette_start&0xf0};
 int const end{env.background.ground_palette_start+env.background.ground_bands-1};
 for(int row{};row<15;++row)for(int index{};index<16;++index){
  int difference{end-(base+index)};if(difference<0)--difference;
  shades[static_cast<size_t>(row)][static_cast<size_t>(index)]=static_cast<uint8_t>(base+index+difference*row/14);
 }
}
void ground_texture::draw(framebuffer &target,vec3<int32_t> const &position,vec3<uint16_t> const &angles,
                          background_view const &view,background_bands const &bands){
 last.reset();
 if(bands.horizon[0].y>view.bounds.bottom&&bands.horizon[1].y>view.bounds.bottom)return;
 int const left{view.centre.x},top{view.centre.y};
 if(top<0)return;
 int const origin_y{view.bounds.top-view.centre.y};
 int const start{std::max(static_cast<int>(wrap_word((static_cast<int>(wrap_word(bands.horizon[0].y))+wrap_word(bands.horizon[1].y))>>1)),view.bounds.top)};
 int rows{view.bounds.bottom-start+1},relative_start{start-top};
 auto const roll{wrap_word(angles.y)};
 if(roll){
  int const cosine{model_cosine(angles.y)?model_cosine(angles.y):1};
  relative_start=divide(wrap_long(static_cast<int64_t>(start-top)*16384),cosine);
  auto edge{rotate({roll>0?160:-160,view.bounds.bottom+origin_y+1},angles.y)};
  rows=edge.y-relative_start;
  int const short_start{wrap_word(relative_start)};
  int const magnitude{short_start==-32768?32767:std::abs(short_start)};
  int const product{wrap_long(static_cast<int64_t>(magnitude)*cosine)};
  int const sine{model_cosine(static_cast<uint16_t>(angles.y-16384))};
  if(roll>0x16a8)stride=divide(0x320000,model_cosine(static_cast<uint16_t>(16384-roll)))+divide(product,sine);
  else if(roll< -0x16a8)stride=divide(0x320000,model_cosine(static_cast<uint16_t>(-16384-roll)))+divide(product,sine);
  else {stride=divide(0x500000,cosine);if(product)stride+=divide(sine,product);}
 }
 if(rows>=320)rows=319;
 if(rows<=0)return;
 if(stride<=0||stride>380)throw std::domain_error{"original ground scratch bitmap exceeds its allocation"};
 int row_start{relative_start-origin_y};
 if(roll)row_start-=wrap_long(static_cast<int64_t>(left-160)*model_cosine(static_cast<uint16_t>(angles.y-16384)))>>14;
 int const pitch{wrap_word(angles.x)};
 // This intentionally retains the original incomplete cache key: viewport,
 // row start and projection shift do not invalidate a sufficiently tall cache.
 if(position.z!=cached_height||pitch!=cached_pitch||rows>cached_count||view.offset.x!=cached_offset){
  cached_height=position.z;cached_pitch=pitch;cached_count=rows;cached_offset=view.offset.x;
  int const height{wrap_long(-static_cast<int64_t>(position.z))};
  for(int i{};i<rows;++i){
   auto ray{rotate({-origin_y-(row_start+i)-1,1<<view.projection_shift},static_cast<uint16_t>(-angles.x))};
   int const distance{ray.x?divide(wrap_long(static_cast<int64_t>(height)*ray.y),ray.x):10000000};
   // 28fbc transposes its already quantized negative-pitch matrix. Building
   // a new positive-angle matrix differs at the cosine table\'s rounding ties.
   auto ground{rotate({height,distance},static_cast<uint16_t>(-angles.x),true)};
   lines[static_cast<size_t>(i)]={
    wrap_long(static_cast<int64_t>(view.offset.x-160)*ground.y)>>view.projection_shift,
    distance,
    wrap_long(static_cast<int64_t>(view.offset.x+159)*ground.y)>>view.projection_shift
   };
  }
 }
 scratch.width=static_cast<unsigned>(stride);scratch.height=static_cast<unsigned>(rows);scratch.flags=0x10;
 scratch.pixels.resize(static_cast<size_t>(rows*stride));
 for(int row{};row<rows;++row){
  auto const &line{lines[static_cast<size_t>(row)]};
  size_t shade{14};
  if(row>=fog_rows){shade=0;while(shade<14&&line[1]>=thresholds[shade])++shade;}
  auto first{rotate({line[0],line[1]},angles.z)},second{rotate({line[2],line[1]},angles.z)};
  first.x=wrap_long(static_cast<int64_t>(first.x)+(static_cast<uint32_t>(position.x)&65535));
  first.y=wrap_long(static_cast<int64_t>(first.y)+(static_cast<uint32_t>(position.y)&65535));
  second.x=wrap_long(static_cast<int64_t>(second.x)+(static_cast<uint32_t>(position.x)&65535));
  second.y=wrap_long(static_cast<int64_t>(second.y)+(static_cast<uint32_t>(position.y)&65535));
  auto delta=[](int32_t end,int32_t begin){return static_cast<uint32_t>(wrap_long(static_cast<int64_t>(wrap_long(static_cast<int64_t>(end)-begin))*204)>>8)&0xffffff;};
  uint32_t const dx{delta(second.x,first.x)},dy{delta(second.y,first.y)};
  uint32_t x{(static_cast<uint32_t>(first.x)<<8)&0xffffff},y{(static_cast<uint32_t>(first.y)<<8)&0xffffff};
  for(int column{};column<stride;++column){
   auto const colour{source[((y>>16)<<8)|(x>>16)]};
   if(colour<240)throw std::runtime_error{"ground source index outside original palette translation"};
   scratch.pixels[static_cast<size_t>(row*stride+column)]=shades[shade][colour-240];
   x=(x+dx)&0xffffff;y=(y+dy)&0xffffff;
  }
 }
 panorama_placement command{0,{0,start},0,false};
 if(roll){
  auto point{rotate({stride>>1,relative_start},angles.y)};
  command.position={-point.x-wrap_word(left-160)+left,point.y+top};
  command.angle=static_cast<uint16_t>(-angles.y);command.rotated=true;
 }
 last=command;
 draw_panorama(target,std::span<bitmap const>{&scratch,1},std::span<panorama_placement const>{&*last,1},view.bounds);
}
}
