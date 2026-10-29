#include "shell/shell_animation.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESSHTTM1")throw std::runtime_error{"fixture version"};
 auto programs{f.integer<uint32_t>()};unsigned total{},draws{};
 for(unsigned p{};p<programs;++p){
  auto name{f.cstring()};auto repeat{f.integer<uint8_t>()};auto count{f.integer<uint32_t>()};
  shell_animation player{read_presentation(r.read("shell0","res32/"+name+".ttm")),repeat!=0};random_state random{17};
  for(unsigned step{};step<count;++step){
   auto elapsed{f.integer<int16_t>()},page{f.integer<int16_t>()};if(page>=0)player.select_page(static_cast<uint16_t>(page));player.advance(elapsed,random);
   auto const &s{player.state()};bool ok{f.integer<uint16_t>()==s.page};ok=(f.integer<uint32_t>()==s.mode)&&ok;
   for(bool flag:{s.wait_latched,s.fade_latched,s.executed})ok=(f.integer<uint8_t>()==flag)&&ok;
   for(int16_t value:{s.remaining,s.period,s.jump,s.branch,s.depth,s.x,s.y})ok=(f.integer<int16_t>()==value)&&ok;
   for(auto word:random.snapshot().words)ok=(f.integer<uint16_t>()==word)&&ok;
   ok=(f.integer<uint8_t>()==random.snapshot().first)&&ok;ok=(f.integer<uint8_t>()==random.snapshot().second)&&ok;
   auto commands{f.integer<uint16_t>()};std::vector<shell_animation_command> actual;
   for(auto const &c:player.commands())if(c.opcode!=0xf02f&&c.opcode!=0x80)actual.push_back(c);
   ok=(commands==actual.size())&&ok;
   for(unsigned i{};i<commands;++i){
    auto opcode{f.integer<uint16_t>()};auto args{f.integer<uint8_t>()};
    ok=(i<actual.size()&&actual[i].opcode==opcode&&actual[i].arguments.size()==args)&&ok;
    for(unsigned j{};j<args;++j){auto value{f.integer<int32_t>()};ok=(i<actual.size()&&j<actual[i].arguments.size()&&actual[i].arguments[j]==value)&&ok;}
   }
   if(!ok){std::cerr<<name<<" step "<<step<<" page "<<s.page<<" mode "<<s.mode<<" wait "<<s.remaining<<'/'<<s.period<<'\n';throw std::runtime_error{"original shell animation mismatch"};}
   ++total;draws+=commands;
  }
 }
 f.require_end();std::cout<<programs<<" shell animations, "<<total<<" updates, "<<draws<<" drawing requests agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
