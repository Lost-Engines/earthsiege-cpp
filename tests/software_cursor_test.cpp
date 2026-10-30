#include "render/software_cursor.h"
#include "resources/resource_store.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCURS01")f.fail("cursor fixture");
 auto const count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto size{f.integer<uint16_t>()};auto name_bytes{f.take(size)};std::string name{name_bytes.begin(),name_bytes.end()};
  auto x{f.integer<int32_t>()},y{f.integer<int32_t>()};auto expected{f.integer<uint32_t>()};auto slash{name.find('/')};
  software_cursor cursor{resources.read(name.substr(0,slash),name.substr(slash+1))};framebuffer frame;frame.clear(123);cursor.draw(frame,x,y);
  if(crc32(frame.pixels())!=expected)f.fail("cursor raster "+name+" at "+std::to_string(x)+","+std::to_string(y));
 }
 f.require_end();std::cout<<count<<" original DCI cursor rasters agree\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
