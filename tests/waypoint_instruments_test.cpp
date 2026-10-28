#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "render/waypoint_instruments.h"

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])};binary_reader file{source};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESHUDW02")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis;std::unique_ptr<waypoint_instruments> instrument;
  for(unsigned int i{};i<count;++i) {
    auto const name{file.cstring()};
    if(chassis!=name) {chassis=name;instrument=std::make_unique<waypoint_instruments>(resources,read_cockpit_layout(resources.read("simvol0","gau/"+name+".GAU")));}
    auto const heading{file.integer<int16_t>()}, tape_heading{file.integer<int16_t>()};
    vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    vec3<int32_t> const point{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    auto const cursor{file.integer<int16_t>()};bool const attached{file.integer<uint8_t>()!=0},present{file.integer<uint8_t>()!=0};
    auto const expected{file.integer<uint32_t>()};
    std::optional<hud_waypoint_state> waypoint;
    if(attached && present && cursor+1<8) waypoint=hud_waypoint_state{vec3<int32_t>{point},static_cast<int16_t>(cursor+1)};
    framebuffer frame;frame.clear(123);instrument->draw(frame,position,static_cast<uint16_t>(heading),static_cast<uint16_t>(tape_heading),waypoint);
    if(crc32(frame.pixels())!=expected) {
      std::ofstream output{"/tmp/es-waypoint-difference.raw",std::ios::binary};output.write(reinterpret_cast<char const *>(frame.pixels().data()),frame.pixels().size());
      file.fail(name+" waypoint paint differs at "+std::to_string(i)+": "+std::to_string(crc32(frame.pixels()))+" expected "+std::to_string(expected));
    }
  }
  file.require_end();std::cout<<count<<" original waypoint frames match\n";return EXIT_SUCCESS;
} catch(std::exception const &e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
