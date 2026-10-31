#include "shell/cockpit_radio.h"
#include "render/weapon_instruments.h"
#include "resources/input_tape.h"
#include <iostream>
#include <fstream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader file{raw};auto magic{file.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESWERAS1")file.fail("fixture version");
 auto tape{read_input_tape(read_file(resources.root()/"TAPES/DEMO1.TAP"))};
 mission_session session{resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
 auto const layout{read_cockpit_layout(resources.read("simvol0","gau/HERCULES.GAU"))};auto const bounds{layout.bounds(0x694)};
 auto count{file.integer<uint32_t>()};size_t total{};
 for(unsigned k{};k<count;++k){
  auto kind{static_cast<weapon_instrument_kind>(file.integer<uint32_t>())};auto steps{file.integer<uint32_t>()};
  random_state random{17};cockpit_radio radio{resources,layout,session,random};weapon_instruments weapon{resources,layout};
  framebuffer backing;backing.clear(123);weapon.draw_retained(backing,3,kind,"TEST",{});weapon.repaint();
  weapon.painted_region=[&](rectangle const &b){radio.overpaint(b);};
  for(unsigned i{};i<steps;++i){
   weapon_instrument_state state;state.value=file.integer<int32_t>();state.disabled=file.integer<uint8_t>();state.selected=file.integer<uint8_t>();
   state.included=file.integer<uint8_t>();state.ready=file.integer<uint8_t>();bool erase{file.integer<uint8_t>()!=0};auto expected{file.integer<uint32_t>()};
   framebuffer frame{backing};weapon.draw_retained(frame,3,kind,"TEST",state);
   if(erase)radio.repaint();
   radio.draw(frame);
   std::vector<uint8_t> crop;for(int y{bounds.top};y<=bounds.bottom;++y)for(int x{bounds.left};x<=bounds.right;++x)crop.push_back(frame.pixels()[y*320+x]);
   if(crc32(crop)!=expected){
    std::ofstream debug{"/tmp/earthsiege-weapon-message.raw",std::ios::binary};
    debug.write(reinterpret_cast<char const *>(crop.data()),static_cast<std::streamsize>(crop.size()));debug.close();
    file.fail("retained weapon/message overlap kind "+std::to_string(k)+" frame "+std::to_string(i));
   }
   ++total;
  }
 }
 file.require_end();std::cout<<total<<" original retained weapon/message overlap rasters match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
