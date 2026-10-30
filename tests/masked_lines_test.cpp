#include <cstdlib>
#include <iostream>
#include "render/line_raster.h"
#include "resources/binary_reader.h"

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=2)return EXIT_FAILURE;
  auto const data{read_file(argv[1])};binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMLIN01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};auto const colour{file.integer<uint8_t>()};
  std::vector<screen_mask> masks(file.integer<uint16_t>());
  for(auto &mask:masks){
    mask.regions.resize(file.integer<uint16_t>());
    for(auto &region:mask.regions){
      region.first_row=file.integer<int16_t>();region.spans.resize(file.integer<uint16_t>());
      region.rectangular=file.integer<uint8_t>()!=0;
      for(auto &span:region.spans){span.left=file.integer<int16_t>();span.right=file.integer<int16_t>();}
    }
    mask.translate(0,0);
  }
  unsigned failures{};
  for(unsigned i{};i<count;++i){
    auto const mask{file.integer<uint16_t>()};
    vec2<int32_t> const a{file.integer<int32_t>(),file.integer<int32_t>()};
    vec2<int32_t> const b{file.integer<int32_t>(),file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame;frame.clip(&masks.at(mask));
    draw_model_line(frame,vec2<int32_t>{a},vec2<int32_t>{b},{0,0,319,199},colour,line_raster_mode::clip_regions);
    if(crc32(frame.pixels())!=expected){
      if(failures<12)std::cerr<<"masked line "<<i<<" differs\n";
      ++failures;
    }
  }
  file.require_end();
  if(failures){std::cerr<<failures<<" masked rasters differ\n";return EXIT_FAILURE;}
  std::cout<<count<<" original masked line rasters agree\n";
  return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
