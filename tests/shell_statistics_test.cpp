#include "render/shell_statistics.h"
#include "shell/game_state.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};shell_statistics panels{resources};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESSTATS1")f.fail("fixture version");
 auto cases{f.integer<uint32_t>()};
 for(unsigned i{};i<cases;++i){
  auto kind{f.integer<uint8_t>()};rectangle b{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
  auto name{f.cstring()};auto state{read_career_state(f)};return_presentation result;
  for(auto &v:result.statistics)v=f.integer<int16_t>();
  result.recovery=f.integer<int32_t>();result.deaths=f.integer<int16_t>();result.recovered_weapons=f.integer<int16_t>();result.technology=f.integer<uint8_t>()!=0;
  auto crc{f.integer<uint32_t>()};framebuffer target;target.clear(37);
  if(kind==0)panels.career(target,b,state,name);else panels.debrief(target,b,state,result);
  if(crc32(target.pixels())!=crc){
   std::ofstream dump{"build/statistics-actual.raw",std::ios::binary};auto const &p{target.pixels()};dump.write(reinterpret_cast<char const *>(p.data()),p.size());
   f.fail("original statistics pixels, case "+std::to_string(i));
  }
 }
 f.require_end();std::cout<<cases<<" original statistics panels agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
