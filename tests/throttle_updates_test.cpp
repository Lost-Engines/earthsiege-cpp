#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "render/instrument_bars.h"

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto bytes{read_file(argv[2])};binary_reader file{bytes};
  auto magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESTHUP01")file.fail("fixture version");
  auto count{file.integer<uint32_t>()};std::string chassis;
  std::unique_ptr<throttle_gauge> gauge;
  unsigned failed{};
  for(uint32_t i{};i<count;++i){
    auto name{file.cstring()};auto speed{file.integer<int32_t>()};
    auto repaint{file.integer<uint8_t>()};auto expected{file.integer<uint32_t>()};
    if(name!=chassis){
      chassis=name;
      gauge=std::make_unique<throttle_gauge>(resources,read_cockpit_layout(resources.read("simvol0","gau/"+name+".GAU")));
    }
    if(repaint)gauge->repaint_lamps();
    framebuffer frame;frame.clear(123);
    gauge->draw_retained_at(frame,speed,gauge->thumb_bounds(0));
    auto actual{crc32(frame.pixels())};
    if(actual!=expected){
      if(failed++<12)std::cerr<<name<<" row "<<i<<" speed "<<speed<<" CRC "<<actual<<" != "<<expected<<'\n';
    }
  }
  file.require_end();
  if(failed)throw std::runtime_error{std::to_string(failed)+" sequential throttle frames differ"};
  std::cout<<count<<" original incremental throttle frames agree\n";
  return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
