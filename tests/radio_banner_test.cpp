#include "render/radio_banner.h"
#include <cstdlib>
#include <iostream>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};radio_banner banner{resources};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESRBAN01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const string{[&]{auto const data{file.take(file.integer<uint16_t>())};return std::string(data.begin(),data.end());}};
  for(uint32_t i{};i<count;++i){
    auto const who{file.integer<int32_t>()};auto const text{string()},expected{string()};
    std::optional<size_t> const speaker{who<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(who)}};
    auto const formatted{banner.format(speaker?std::array{"DUGGAN","RIGGS","GLENN"}[*speaker]:"XXXX",text)};
    if(formatted!=expected)file.fail("radio formatted label "+std::to_string(i));
    auto const b{banner.bounds(formatted)};
    if(b.left!=file.integer<int32_t>() || b.top!=file.integer<int32_t>() || b.right!=file.integer<int32_t>() || b.bottom!=file.integer<int32_t>())file.fail("radio bounds");
    auto const crc{file.integer<uint32_t>()};framebuffer frame;frame.clear(123);banner.draw(frame,formatted,speaker);
    if(crc32(frame.pixels())!=crc)file.fail("radio pixels "+std::to_string(i));
  }
  file.require_end();std::cout<<count<<" original radio labels and rasters match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
