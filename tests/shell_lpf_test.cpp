#include "resources/lpf_animation.h"
#include "resources/resource_store.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESLPF001")f.fail("fixture version");
 auto cases{f.integer<uint16_t>()};unsigned frames{};
 for(unsigned c{};c<cases;++c){
  auto name{f.cstring()};auto transparent{f.integer<uint16_t>()},pattern{f.integer<uint16_t>()},count{f.integer<uint16_t>()},fps{f.integer<uint16_t>()};
  lpf_animation anim{resources.read("shell0","anm/"+name+".anm")};
  if(anim.frames()!=count||anim.frames_per_second()!=fps||anim.width()!=320||anim.height()!=200)f.fail("original header mismatch");
  std::vector<uint8_t> base(64000);
  for(unsigned y{};y<200;++y)for(unsigned x{};x<320;++x)base[y*320+x]=static_cast<uint8_t>(pattern?(x/7+y/5*17+1)&255:37);
  auto pixels{base};
  for(unsigned i{};i<count;++i){
   anim.decode(i,pixels,transparent?bytes{base}:bytes{});
   auto expected{f.integer<uint32_t>()};
   if(crc32(pixels)!=expected){std::ofstream out{"build/lpf-actual.raw",std::ios::binary};out.write(reinterpret_cast<char const *>(pixels.data()),pixels.size());f.fail(name+" frame "+std::to_string(i)+" original pixels differ");}
   ++frames;
  }
 }
 f.require_end();std::cout<<frames<<" complete original LPF/ANM frames agree, including background restoration\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
