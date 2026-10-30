#include <cstdlib>
#include <iostream>
#include "shell/cockpit_hdd.h"
#include "resources/name_table.h"

int main(int argc,char **argv)try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const data{read_file(argv[2])};binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESHCLK01")file.fail("fixture version");
  auto const script{read_mission_script(file.take(file.integer<uint32_t>()))};
  auto const roster{read_player_roster(file.take(file.integer<uint32_t>()))};
  auto const counters{read_mission_counters(file.take(file.integer<uint32_t>()))};
  mission_session session{resources,script,roster,random_state{},counters};
  auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  auto const art{read_cockpit(resources,names.at(static_cast<size_t>(session.player_chassis())))};
  pointer_controls input;
  cockpit_hdd display{resources,art.layout,session,input,session.interface_random()};
  auto camera{session.make_camera(camera_mode::attached)};
  auto const &origin{art.windows[1].instrument_origin};
  display.command(65,session,input,120000);
  for(unsigned i{};i<12;++i)display.update(session,input,camera,0,origin[0],origin[1]);
  auto const count{file.integer<uint32_t>()};unsigned failures{};
  for(unsigned i{};i<count;++i){
    auto const tick{file.integer<uint32_t>()};auto const key{file.integer<uint16_t>()};
    auto const page{file.integer<uint8_t>()};
    auto const map{file.integer<uint32_t>()},damage{file.integer<uint32_t>()};
    bool const blink{file.integer<uint8_t>()!=0};
    if(key)display.command(key,session,input,120000);
    display.update(session,input,camera,tick,origin[0],origin[1]);
    auto const &actual{display.refresh_state()};
    if(actual.map_deadline!=map||actual.damage_deadline!=damage||actual.blink_phase!=blink){
      if(failures<12)std::cerr<<"HDD "<<i<<" page "<<unsigned(page)<<" tick "<<tick<<": map "<<actual.map_deadline<<'/'<<map<<", damage "<<actual.damage_deadline<<'/'<<damage<<", blink "<<actual.blink_phase<<'/'<<blink<<'\n';
      ++failures;
    }
  }
  file.require_end();
  if(failures){std::cerr<<failures<<" refresh states differ\n";return EXIT_FAILURE;}
  std::cout<<count<<" original MAP/DAMAGE refresh states match\n";
  return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
