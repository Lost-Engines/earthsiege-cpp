#include <cstdlib>
#include <fstream>
#include <iostream>
#include "shell/cockpit_hdd.h"
#include "resources/name_table.h"

int main(int argc,char **argv)try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader file{data};
  auto const magic{file.take(8)};if(std::string(magic.begin(),magic.end())!="ESHDD001")file.fail("fixture version");
  auto const script{read_mission_script(file.take(file.integer<uint32_t>()))};
  auto const roster{read_player_roster(file.take(file.integer<uint32_t>()))};
  auto const counters{read_mission_counters(file.take(file.integer<uint32_t>()))};
  mission_session session{resources,script,roster,random_state{},counters};
  auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  auto const art{read_cockpit(resources,names.at(static_cast<size_t>(session.player_chassis())))};
  pointer_controls input;cockpit_hdd display{resources,art.layout,session,input,session.interface_random()};
  auto camera{session.make_camera(camera_mode::attached)};auto const &origin{art.windows[1].instrument_origin};
  display.update(session,input,camera,0,origin[0],origin[1]);
  auto const regions{file.integer<uint32_t>()};std::vector<rectangle> mask;
  for(unsigned i{};i<regions;++i)mask.push_back({file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()});
  auto const count{file.integer<uint32_t>()};unsigned failures{};
  for(unsigned i{};i<count;++i){
    auto const key{file.integer<uint16_t>()};int const command{file.integer<int32_t>()},wingmate{file.integer<int32_t>()};
    display.command(key,session,input,120000);display.update(session,input,camera,0,origin[0],origin[1]);
    framebuffer frame;art.draw(frame,cockpit_view::lower);display.draw(frame,input,-origin[0],-origin[1]);
    auto expected{frame};size_t different{};
    for(auto const &b:mask)for(int y{b.top};y<=b.bottom;++y)for(int x{b.left};x<=b.right;++x){
      auto const pixel{file.integer<uint8_t>()};expected.pixel(x,y,pixel);
      if(frame.pixels()[static_cast<size_t>(y*320+x)]!=pixel)++different;
    }
    if(command!=display.state().command || wingmate!=display.state().wingmate){std::cerr<<"HDD "<<i<<" control state differs: "<<display.state().command<<'/'<<command<<", "<<display.state().wingmate<<'/'<<wingmate<<'\n';++failures;}
    if(different){if(!failures){std::ofstream a{"/tmp/es-hdd-actual.raw",std::ios::binary},b{"/tmp/es-hdd-expected.raw",std::ios::binary};a.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());b.write(reinterpret_cast<char const*>(expected.pixels().data()),expected.pixels().size());}++failures;std::cerr<<"HDD "<<i<<" key "<<key<<": "<<different<<" pixels differ\n";}
  }
  file.require_end();if(failures)return EXIT_FAILURE;
  std::cout<<count<<" assembled original cockpit HDD frames match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
