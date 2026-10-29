#include "shell/speech_selection.h"
#include "resources/binary_reader.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;auto data{read_file(argv[1])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESMECHN1")f.fail("fixture version");auto cases{f.integer<uint32_t>()};
 for(unsigned i{};i<cases;++i){
  auto condition{f.integer<int16_t>()},technology{f.integer<int16_t>()};auto recovery{f.integer<int32_t>()};
  auto deaths{f.integer<int16_t>()},seed{f.integer<int16_t>()};random_state random{seed};
  auto choice{select_mechanic_return(condition,technology!=0,recovery,deaths,random)};bool ok{true};
  for(auto entry:choice)ok=(f.integer<int16_t>()==entry)&&ok;
  for(auto word:random.snapshot().words)ok=(f.integer<uint16_t>()==word)&&ok;
  ok=(f.integer<uint8_t>()==random.snapshot().first)&&ok;ok=(f.integer<uint8_t>()==random.snapshot().second)&&ok;
  if(!ok)f.fail("mechanic selection "+std::to_string(i));
 }
 f.require_end();std::cout<<cases<<" original mechanic response chains and RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
