#include <cstdlib>
#include <format>
#include <iostream>
#include "resources/resource_store.h"
#include "simulation/herc_body.h"

int main(int argc,char **argv)try{
  using earthsiege::binary_reader;
  using earthsiege::herc_body;
  using earthsiege::herc_controls;
  using earthsiege::read_file;
  using earthsiege::read_herc_definition;
  using earthsiege::read_model;
  using earthsiege::read_weapon_catalogue;
  using earthsiege::resource_store;if(argc != 3)return EXIT_FAILURE;resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader f{data};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESHRST01")f.fail("fixture version");
  auto const weapons{read_weapon_catalogue(resources.read("simvol0","dat/WEAPONS.DAT"))};auto const templates{read_model(resources.read("simvol0","dts/MECHWPNS.DTS"))};
  auto const count{f.integer<uint16_t>()};size_t steps{},sounds{};
  for(unsigned int model{};model < count;++model){
    auto const name{f.cstring()};auto const definition{read_herc_definition(resources,name)};
    std::vector<int16_t> equipment(f.integer<uint16_t>());for(auto &w:equipment)w = f.integer<int16_t>();
    vec3<int32_t> const position{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};vec3<uint16_t> const angles{f.integer<uint16_t>(),f.integer<uint16_t>(),f.integer<uint16_t>()};
    herc_body body{definition,weapons,templates,equipment,position,angles};auto const frames{f.integer<uint16_t>()};
    for(unsigned int frame{};frame < frames;++frame){
      herc_controls const input{f.integer<int16_t>(),f.integer<int16_t>(),f.integer<int16_t>(),f.integer<int16_t>()};auto const ratio{f.integer<int16_t>()};body.step(input,ratio);auto const events{body.footstep_sounds()};
      if(events.size() != f.integer<uint16_t>())throw std::runtime_error{std::format("{} frame {}: sound count",name,frame)};
      for(auto const id:events)if(id != f.integer<uint16_t>())throw std::runtime_error{"sound id"};
      ++steps;sounds += events.size();
    }
  }
  f.require_end();std::cout << steps << " original articulated HERC updates and " << sounds << " footsteps match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr << e.what() << '\n';return EXIT_FAILURE;}
