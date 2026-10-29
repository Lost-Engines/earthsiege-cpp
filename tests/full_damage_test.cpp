#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/full_damage_display.h"
#include "resources/herc_definition.h"
#include "resources/name_table.h"
#include "simulation/weapon_display.h"
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader f{data};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESFDAM01")f.fail("fixture version");full_damage_display display{resources};framebuffer frame;
  auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};auto const definitions{read_weapon_catalogue(resources.read("simvol0","dat/WEAPONS.DAT"))};auto const models{read_model(resources.read("simvol0","dts/MECHWPNS.DTS"))};
  auto const rect{[&]{return rectangle{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};}};
  auto const chassis{f.integer<uint32_t>()};size_t failures{},frames{};
  for(size_t c{};c<chassis;++c){auto const model{f.integer<int16_t>()};auto const bounds{rect()},listing{rect()};auto const count{f.integer<uint16_t>()};std::vector<int16_t> equipment(count),values(count);
    for(auto &v:equipment)v=f.integer<int16_t>();for(auto &v:values)v=f.integer<int16_t>();
    auto const definition{read_herc_definition(resources,names.at(static_cast<size_t>(model)))};weapon_bank const bank{definitions,models,definition.mounts,equipment,values};full_damage_state state;state.model=model;
    for(auto const &w:bank.slots())state.weapons.push_back(w?monitored_weapon{w->definition,0,weapon_display_name(*w,definitions)}:monitored_weapon{});
    auto const diagrams{display.diagrams(state)};auto const mounts{f.integer<uint16_t>()};if(mounts!=diagrams.size())f.fail("mount count");
    for(auto const &d:diagrams){std::array<int,9> const got{d.frame,d.x,d.y,d.bounds.left,d.bounds.top,d.bounds.right,d.bounds.bottom,d.flags,static_cast<int>(d.slot)};for(auto v:got)if(v!=f.integer<int32_t>())f.fail("weapon diagram differs chassis "+std::to_string(c));}
    auto const cases{f.integer<uint32_t>()};
    for(size_t i{};i<cases;++i){state.model=f.integer<int16_t>();state.mode=f.integer<int16_t>();state.scroll=f.integer<int16_t>();state.subject=f.integer<int16_t>();state.no_information=f.integer<uint8_t>();
      for(auto &v:state.armour)v=f.integer<int16_t>();for(auto &v:state.internal)v=f.integer<int16_t>();for(size_t k{};k<9;++k){auto const v{f.integer<int16_t>()};if(k<state.weapons.size())state.weapons[k].damage=v;}
      auto const expected{f.integer<uint32_t>()};frame.clear(123);display.draw(frame,bounds,listing,state);++frames;
      if(crc32(frame.pixels())!=expected){if(failures<8)std::cerr<<"damage "<<c<<'/'<<i<<" differs\n";if(!failures){std::ofstream out{"/tmp/es-full-damage-difference.raw",std::ios::binary};out.write(reinterpret_cast<char const*>(frame.pixels().data()),frame.pixels().size());}++failures;}
    }
  }
  f.require_end();if(failures)f.fail(std::to_string(failures)+" / "+std::to_string(frames)+" damage pages differ");
  std::cout<<frames<<" original full damage pages match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
