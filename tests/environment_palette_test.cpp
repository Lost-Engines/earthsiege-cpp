#include "render/environment_palette.h"
#include "resources/resource_store.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESENPAL1")f.fail("fixture version");
 auto count{f.integer<uint16_t>()};unsigned cases{};
 for(unsigned world_index{};world_index<count;++world_index){
  auto world{f.integer<uint16_t>()},steps{f.integer<uint16_t>()};
  auto env{read_world_environment(resources.read("simvol0","wld/WORLD"+std::to_string(world)+".WLD"))};
  auto colours{read_palette(resources.read("simvol0","dpl/WORLD"+std::to_string(world)+".DPL"))};
  environment_palette reflection{env};
  for(unsigned step{};step<steps;++step){
   auto pitch{f.integer<uint16_t>()};auto override{f.integer<int16_t>()};bool impact{f.integer<uint8_t>()!=0};
   reflection.override_band(override);reflection.update(colours,pitch,true,impact);
   if(reflection.pending_override()!=-1)f.fail("one-shot palette override");
   for(size_t index{64};index<80;++index)for(unsigned c{};c<3;++c)if(colours.dac_colours[index][c]!=f.integer<uint8_t>())f.fail("original reflection palette");
   ++cases;
  }
 }
 f.require_end();std::cout<<cases<<" original pitch, one-shot override and impact palettes agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
