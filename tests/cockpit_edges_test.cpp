#include "shell/cockpit_transition.h"
#include "resources/binary_reader.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;auto raw{read_file(argv[1])};binary_reader file{raw};
 auto magic{file.take(8)};if(std::string(magic.begin(),magic.end())!="ESEDGE01")file.fail("format");
 auto count{file.integer<uint32_t>()};
 for(uint32_t i{};i<count;++i){
  auto view{file.integer<int32_t>()},x{file.integer<int32_t>()},y{file.integer<int32_t>()};
  auto cursor{file.integer<int32_t>()},request{file.integer<int32_t>()};int actual{-1};
  auto bounds{cockpit_edge_bounds(view)};
  for(size_t b{};b<bounds.size();++b)if(bounds[b].contains(x,y)){actual=cockpit_edge_request(view,b);break;}
  if(cockpit_edge_cursor(view,x,y)!=cursor||actual!=request)throw std::runtime_error{"edge case "+std::to_string(i)};
 }
 std::cout<<count<<" original edge hover and request cases agree\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
