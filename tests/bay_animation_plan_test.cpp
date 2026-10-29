#include "shell/bay_animation_plan.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
bay_animation_state read_state(binary_reader &f){
 bay_animation_state s;for(auto &v:s.owners)v=f.integer<int16_t>();for(auto &v:s.left)v=f.integer<int16_t>();for(auto &v:s.right)v=f.integer<int16_t>();return s;
}
void compare(binary_reader &f,bay_animation_plan const &plan,random_state const &random){
 auto expected{read_state(f)};auto const &s{plan.state()};
 bool ok{s.owners==expected.owners&&s.left==expected.left&&s.right==expected.right};
 for(auto w:random.snapshot().words)ok=(f.integer<uint16_t>()==w)&&ok;
 ok=(f.integer<uint8_t>()==random.snapshot().first)&&ok;ok=(f.integer<uint8_t>()==random.snapshot().second)&&ok;
 if(!ok)throw std::runtime_error{"original bay animation plan mismatch"};
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto const original{read_hangar(r.read("shell0","car/sm_init.dat"))};
 auto const prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBAYAN1")throw std::runtime_error{"fixture version"};
 auto initial{f.integer<uint32_t>()},changes{f.integer<uint32_t>()};
 for(unsigned c{};c<initial;++c){
  auto h{original};career_state crew;auto seed{f.integer<uint16_t>()};auto mask{f.integer<uint8_t>()};
  h.selected=f.integer<int16_t>();h.construction_slot=f.integer<int16_t>();h.construction_remaining=f.integer<int16_t>();crew.selected_crew=f.integer<int16_t>();
  auto campaign{f.integer<uint8_t>()},kind{f.integer<uint8_t>()};crew.current=f.integer<uint8_t>();
  for(auto &member:crew.crew)member.hangar_slot=f.integer<int16_t>();
  std::erase_if(h.hercs,[&](auto const &herc){return !(mask&(1<<herc.slot));});
  bay_animation_plan plan;random_state random{static_cast<int16_t>(seed)};plan.reset(h,crew,campaign!=0,kind,prices,random);
  bool const selected{f.integer<int16_t>()==h.selected},cursor{f.integer<int16_t>()==crew.crew_cursor};
  try{compare(f,plan,random);}catch(...){std::cerr<<"initial "<<c<<'\n';throw;}
  if(!selected||!cursor)throw std::runtime_error{"bay initialization cursor mismatch"};
 }
 for(unsigned c{};c<changes;++c){
  auto h{original};auto seed{f.integer<uint16_t>()};h.selected=f.integer<int16_t>();h.construction_slot=f.integer<int16_t>();h.construction_remaining=f.integer<int16_t>();
  auto slot{f.integer<int16_t>()},role{f.integer<int16_t>()};bay_animation_plan plan{read_state(f)};random_state random{static_cast<int16_t>(seed)};
  plan.change(slot,role,h,prices,random);
  try{compare(f,plan,random);}catch(...){std::cerr<<"change "<<c<<'\n';throw;}
 }
 f.require_end();std::cout<<initial<<" bay initializations and "<<changes<<" changes agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
