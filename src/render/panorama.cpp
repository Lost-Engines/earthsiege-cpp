#include "panorama.h"
#include "render/texture_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
std::vector<panorama_placement> place_panorama(std::span<bitmap const> images,background_bands const &bands,background_view const &view,uint16_t heading,int horizon_offset){
 std::vector<panorama_placement> result;
 if(images.empty()||(bands.horizon[0].y+horizon_offset<=0&&bands.horizon[1].y+horizon_offset<=0))return result;
 int const count{static_cast<int>(images.size())},width{static_cast<int>(images.front().width)};
 if(!width)return result;
 auto const divisor{(static_cast<uint64_t>(65536/count)<<16)/static_cast<unsigned>(width)};
 int x{static_cast<int>((uint64_t{heading}<<16)/divisor)};
 int const left{view.bounds.left+view.offset.x};
 while(left<x)x-=count*width;
 int y{bands.horizon[0].y},dx{width},dy{},extra_x{},extra_y{};
 int const sine{model_cosine(static_cast<uint16_t>(view.roll-16384))},cosine{model_cosine(view.roll)};
 if(view.roll){
  dy=(-sine*width)>>14;dx=(cosine*width)>>14;
  extra_x=(sine*horizon_offset)>>14;extra_y=(cosine*horizon_offset)>>14;
  int const centre_x{((bands.horizon[0].x+bands.horizon[1].x)>>1)+view.offset.x};
  int const centre_y{((bands.horizon[0].y+bands.horizon[1].y)>>1)+view.offset.y};
  int const relative{x-160};x=((relative*cosine)>>14)+centre_x;y=((-relative*sine)>>14)+centre_y;
 }
 // The original strip walk cannot make forward progress at inverted roll.
 if(dx<=0)throw std::domain_error{"original panorama has nonpositive horizontal step"};
 size_t image{};
 while(x+dx<left){x+=dx;y+=dy;if(++image==images.size())image=0;}
 for(int drawn{};x<left+320&&drawn<count;++drawn){
  int const height{static_cast<int>(images[image].height)};
  vec2<int32_t> position{x-view.offset.x,y-view.offset.y};
  if(view.roll){position.x+=extra_x-((height*sine)>>14);position.y+=extra_y-((height*cosine)>>14);}
  else position.y+=horizon_offset-height;
  result.push_back({image,vec2<int32_t>{position},static_cast<uint16_t>(-view.roll),view.roll!=0});
  x+=dx;y+=dy;if(++image==images.size())image=0;
 }
 return result;
}
void draw_panorama(framebuffer &target,std::span<bitmap const> images,std::span<panorama_placement const> placements,rectangle bounds){
 for(auto const &p:placements){
  auto const &image{images[p.frame]};int const width{static_cast<int>(image.width)},height{static_cast<int>(image.height)};
  if(!p.rotated){
   for(int y{std::max(bounds.top,p.position.y)};y<=std::min(bounds.bottom,p.position.y+height-1);++y)
    for(int x{std::max(bounds.left,p.position.x)};x<=std::min(bounds.right,p.position.x+width-1);++x){
     auto const colour{image.pixels.at(static_cast<size_t>((y-p.position.y)*width+x-p.position.x))};
     if(colour)target.pixel(x,y,colour);
    }
   continue;
  }
  // Original 0x76630 rotates a width-by-height quad; its texture corners end
  // at width-1 / height-1. The affine mapper supplies the native transparency.
  int const c{model_cosine(p.angle)},s{model_cosine(static_cast<uint16_t>(p.angle-16384))};
  std::array<vec2<int32_t>,4> const corners{vec2<int32_t>{0,0},vec2<int32_t>{width,0},vec2<int32_t>{width,height},vec2<int32_t>{0,height}};
  std::array<projected_vertex,4> vertices;
  for(size_t i{};i<vertices.size();++i){
   auto const &v{corners[i]};
   vertices[i].position={p.position.x+wrap_long((static_cast<int64_t>(v.x)*c-static_cast<int64_t>(v.y)*s+8192)>>14),
                         p.position.y+wrap_long((static_cast<int64_t>(v.x)*s+static_cast<int64_t>(v.y)*c+8192)>>14)};
   vertices[i].texture={(i==1||i==2)?width-1:0,i>=2?height-1:0};
  }
  draw_textured_polygon(target,vertices,image,bounds,{},target.clip()?texture_scan_direction::rows:texture_scan_direction::columns);
 }
}
} // namespace earthsiege
