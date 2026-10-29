#include "resources/input_options.h"
#include "resources/binary_reader.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESINCFG1")f.fail("fixture version");
 auto configs{f.integer<uint32_t>()};
 for(unsigned i{};i<configs;++i){auto text{f.take(f.integer<uint32_t>())};auto s{read_keyjoy(std::string_view{reinterpret_cast<char const *>(text.data()),text.size()})};
  for(auto b:{s.tilt_reverse,s.backturn_reverse,s.missile_reverse,s.invalid_token})if(b!=(f.integer<uint8_t>()!=0))f.fail("KEYJOY case "+std::to_string(i));
 }
 auto hats{f.integer<uint32_t>()};
 for(unsigned i{};i<hats;++i){auto view{f.integer<uint8_t>()},bits{f.integer<uint8_t>()};std::array<uint8_t,4> hat;for(unsigned k{};k<4;++k)hat[k]=static_cast<uint8_t>((bits>>k)&1);
  if(cockpit_hat_request(view,hat)!=f.integer<int8_t>())f.fail("hat transition "+std::to_string(i));
 }
 f.require_end();std::cout<<configs<<" original KEYJOY files and "<<hats<<" hat transitions agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
