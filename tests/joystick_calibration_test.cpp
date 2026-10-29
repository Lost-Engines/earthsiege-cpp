#include "shell/joystick_calibration.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto bytes_{read_file(argv[1])};binary_reader f{bytes_};auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESJCAL01")f.fail("fixture version");
 auto cases{f.integer<uint32_t>()};unsigned changes{},writes{};
 for(unsigned c{};c<cases;++c){
  auto model{f.integer<uint8_t>()};bool const pedals{f.integer<uint8_t>()!=0},present{f.integer<uint8_t>()!=0};
  auto initial{read_joystick_calibration(f.take(64))};std::array<joystick_calibration,2> devices{initial,initial};
  joystick_calibration_state dialog{model,pedals,{present&&model!=0,present&&(pedals||model==2)},devices};unsigned case_writes{};
  auto compare{[&](unsigned step){
   for(size_t i{};auto v:dialog.state()){auto expected{f.integer<int32_t>()};if(v!=expected)f.fail("calibration local "+std::to_string(c)+"/"+std::to_string(step)+"/"+std::to_string(i)+" "+std::to_string(v)+" != "+std::to_string(expected));++i;}
   for(auto const &d:devices)if(d!=read_joystick_calibration(f.take(64)))f.fail("calibration live "+std::to_string(c)+"/"+std::to_string(step));
   if(dialog.backup()!=read_joystick_calibration(f.take(64)))f.fail("calibration backup "+std::to_string(c)+"/"+std::to_string(step));
   if(case_writes!=f.integer<uint32_t>())f.fail("calibration save count");
  }};
  auto count{f.integer<uint16_t>()};compare(0);
  for(unsigned i{};i<count;++i){
   auto button{f.integer<uint8_t>()};auto key{f.integer<uint16_t>()};auto value{f.integer<int32_t>()};
   if(button)dialog.activate(button-1);
   if(key==1)dialog.cancel();else if(key){
    std::array<int32_t,4> raw{};for(unsigned j{};j<4;++j)raw[j]=value+int(j)*11;
    if(dialog.sample(raw)){++case_writes;++writes;}
   }
   compare(i+1);++changes;
  }
  if(!dialog.done())f.fail("calibration Done");
 }
 f.require_end();std::cout<<cases<<" original calibration sequences / "<<changes<<" inputs / "<<writes<<" saves agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
