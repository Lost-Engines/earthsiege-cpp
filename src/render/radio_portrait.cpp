#include "radio_portrait.h"
#include <stdexcept>

namespace earthsiege {
namespace {constexpr std::array<size_t,28> reduced_frames{1,0,1,4,2,4,5,4,2,4,4,3,3,3,4,5,5,2,4,4,4,1,2,4,6,6,6,7};}
radio_portrait::radio_portrait(resource_store const &resources,size_t const pilot,bool const reduced_detail):reduced{reduced_detail}{
  auto const name{"PILOT"+std::to_string(pilot)};
  images=read_bitmaps(resources.read("simvol0","dba/"+name+(reduced?"S":"")+".DBA"));
  auto const data{resources.read("simvol0","ofs/"+name+".OFS")};binary_reader file{data,"portrait offsets"};
  for(size_t i{};i<27;++i){
    auto const index{file.integer<uint32_t>()};if(index>=27)file.fail("portrait offset index");
    offsets[index]={file.integer<int32_t>(),file.integer<int32_t>()};
  }
  // Death frame 27 has no loaded offset; use the zero values in the original
  // captured allocation rather than reading indeterminate memory.
  // The original reads exactly 27 records; some OFS files retain later data.
  auto const rest{file.take(file.remaining())};unused_offset_data.assign(rest.begin(),rest.end());
}
void radio_portrait::update(size_t const mouth_frame,random_state &random){
  if(mouth_frame>=24 && mouth_frame!=27)throw std::out_of_range{"portrait mouth frame"};
  mouth=mouth_frame;
  backdrop=reduced?24:24+static_cast<uint32_t>(static_cast<int32_t>(random.next()))%3;
}
void radio_portrait::draw(framebuffer &frame,rectangle const &bounds)const{
  scoped_framebuffer_clip clip{frame,nullptr};frame.fill(bounds,63);
  auto const blit{[&](size_t const image,size_t const position){
    auto const &source{images.at(image)};auto const &p{offsets.at(position)};
    for(unsigned int y{};y<source.height;++y)for(unsigned int x{};x<source.width;++x){
      auto const colour{source.pixels[y*source.width+x]};int const px{bounds.left+p[0]+static_cast<int>(x)},py{bounds.top+p[1]+static_cast<int>(y)};
      if(colour && bounds.contains(px,py))frame.pixel(px,py,colour);
    }
  }};
  blit(reduced?6:backdrop,backdrop);
  blit(reduced?reduced_frames.at(mouth):mouth,mouth);
}
} // namespace earthsiege
