#include "shell/mission_alert.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};auto m{f.take(8)};
 if(std::string(m.begin(),m.end())!="ESALTI01")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto message{f.integer<uint8_t>()};bool wcs{f.integer<uint8_t>()!=0};int initial{f.integer<int8_t>()};
  auto scan{f.integer<uint16_t>()};bool fire{f.integer<uint8_t>()!=0},next{f.integer<uint8_t>()!=0};
  bool consumed{f.integer<int32_t>()!=0};int focus{f.integer<int8_t>()};auto done{f.integer<uint8_t>()};
  mission_alert alert{resources,message,initial};auto out{alert.input(scan,fire,next,wcs)};
  if(out.consumed!=consumed||alert.focused()!=focus||out.response.has_value()!=(done!=0)||(done&&*out.response!=(done&1)))f.fail("alert input "+std::to_string(i));
 }
 f.require_end();std::cout<<count<<" original modal command/button cases agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
