#include "shell/speech_selection.h"
#include "resources/resource_store.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader f{data};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESSPSEL1")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto mode{f.integer<int16_t>()},chapter{f.integer<int16_t>()},entry{f.integer<int16_t>()},grade{f.integer<int16_t>()},kind{f.integer<int16_t>()},active{f.integer<int16_t>()},seed{f.integer<int16_t>()};
  auto file{f.cstring()};auto number{f.integer<int16_t>()},speaker{f.integer<int16_t>()},selected{f.integer<int16_t>()},song{f.integer<int16_t>()};
  random_state random{seed};auto result{select_shell_speech(mode,chapter,entry,grade,kind,active!=0,random)};
  std::string stem{speaker==3?"G0":speaker==2?"T"+std::to_string(number):std::string{number&1?"D":"C"}+std::to_string(number/2)};
  if(normalise_path(file)!=normalise_path(result.text_file)||result.entry!=selected||result.song!=song||normalise_path(result.list_file)!=normalise_path("snc/"+stem+"_files.dat"))f.fail("original speech selection case "+std::to_string(i));
  for(auto word:random.snapshot().words)if(word!=f.integer<uint16_t>())f.fail("speech RNG word");
  if(random.snapshot().first!=f.integer<uint8_t>()||random.snapshot().second!=f.integer<uint8_t>())f.fail("speech RNG cursor");
 }
 f.require_end();std::cout<<count<<" original shell speech selections and RNG states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
