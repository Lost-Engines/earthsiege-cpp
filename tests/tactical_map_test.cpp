#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/tactical_map.h"
#include "resources/resource_store.h"
int main(int argc,char **argv)try {
  using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;resource_store resources{argv[1]};
  auto const data{read_file(argv[2])};binary_reader file{data};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESTMAP01")file.fail("fixture version");
  std::array<int16_t,10> header;for(auto &v:header)v=file.integer<int16_t>();
  mission_header const mission{.terrain{static_cast<terrain_kind>(header[0])},.zone{header[1]},.world_variant{header[8]},.palette_variant{header[9]}};
  scenery_world world{resources,mission};framebuffer frame;tactical_map display{resources,frame,world};
  auto const rect{[&]{return rectangle{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};}};
  auto const bounds{rect()};auto const limits{rect()};auto const initial{file.integer<int32_t>()};
  if(initial_tactical_height(limits,bounds)!=initial)file.fail("initial map height");
  auto const count{file.integer<uint32_t>()};unsigned failures{};
  for(unsigned i{};i<count;++i){
    tactical_map_state state;state.mission_bounds=limits;state.camera={file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    state.ticks=file.integer<uint32_t>();state.failed=file.integer<uint8_t>();auto const contacts{file.integer<uint16_t>()};
    for(unsigned j{};j<contacts;++j){
      tactical_contact c;c.kind=file.integer<int16_t>();c.frame=file.integer<int16_t>();c.size=file.integer<int16_t>();c.side=file.integer<uint8_t>();
      c.dead=file.integer<uint8_t>();c.hidden=file.integer<uint8_t>();c.detected=file.integer<uint8_t>();c.last_position_unavailable=file.integer<uint8_t>();file.integer<uint8_t>();
      c.heading=file.integer<uint16_t>();c.position={file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      c.last_position={file.integer<int32_t>(),file.integer<int32_t>()};c.selected=file.integer<uint8_t>();state.contacts.push_back(c);
    }
    auto const expected{file.integer<uint32_t>()};frame.clear(123);display.draw(bounds,state);
    if(crc32(frame.pixels())!=expected){if(!failures){std::ofstream out{"/tmp/es-tactical-difference.raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}++failures;std::cerr<<"map "<<i<<" differs\n";}
  }
  file.require_end();if(failures)file.fail(std::to_string(failures)+" tactical map frames differ");
  std::cout<<count<<" original tactical map frames match\n";return EXIT_SUCCESS;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
