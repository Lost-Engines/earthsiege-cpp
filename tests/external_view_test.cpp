#include "render/external_view.h"
#include <iostream>
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=3)return 1;
 resource_store resources{argv[1]};external_view_artwork artwork{resources};
 auto raw{read_file(argv[2])};binary_reader file{raw};auto signature{file.take(8)};
 if(std::string(signature.begin(),signature.end())!="ESEXT001")file.fail("format");
 auto const count{file.integer<uint32_t>()};
 for(uint32_t i{};i<count;++i){
  auto const control{file.integer<uint8_t>()};auto expected{file.take(320*29)};
  framebuffer frame;frame.clear();artwork.draw(frame,control!=0);
  size_t errors{};for(size_t p{};p<expected.size();++p)if(expected[p]!=frame.pixels()[320*171+p])++errors;
  if(errors)throw std::runtime_error{"external control "+std::to_string(control)+": "+std::to_string(errors)+" different pixels"};
 }
 file.require_end();std::cout<<count<<" original external status rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
