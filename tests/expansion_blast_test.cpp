#include "resources/resource_store.h"
#include "simulation/shield_power.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader r{data};auto magic{r.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESEXSHL1")throw std::runtime_error{"fixture magic"};
 auto cases{r.integer<uint16_t>()};
 for(unsigned i{};i<cases;++i){shield_power shields;shields.front=r.integer<int16_t>();shields.rear=r.integer<int16_t>();auto amount{r.integer<int16_t>()};auto front{r.integer<int16_t>()};auto remaining{absorb_blast(shields,front!=0,amount)};auto expected_front{r.integer<int16_t>()},expected_rear{r.integer<int16_t>()},expected_remaining{r.integer<int16_t>()};
  if(shields.front!=expected_front||shields.rear!=expected_rear||remaining!=expected_remaining)throw std::runtime_error{"native blast shield case "+std::to_string(i)};
 }
 r.require_end();std::cout<<cases<<" original radial shield cases agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
