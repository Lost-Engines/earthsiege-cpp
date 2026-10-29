#include "shell/crew_assignment.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCREW01")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned events{};
 for(unsigned sequence{};sequence<cases;++sequence){
  hangar h;career_state c;auto mask{f.integer<int16_t>()};h.construction_slot=f.integer<int16_t>();h.selected=f.integer<int16_t>();c.crew_cursor=f.integer<int16_t>();
  for(int16_t i{};i<8;++i)if(mask&(1<<i)){player_herc p;p.slot=i;h.hercs.push_back(p);}
  for(auto &m:c.crew)m.hangar_slot=f.integer<int16_t>();
  crew_assignment edit{h,c};
  auto compare=[&](){
   std::array<int16_t,11> actual{h.selected,c.crew_cursor,c.crew[0].hangar_slot,c.crew[1].hangar_slot,c.crew[2].hangar_slot,c.crew[3].hangar_slot};
   auto state{edit.state()};std::copy(state.begin(),state.end(),actual.begin()+6);
   for(size_t i{};i<actual.size();++i){auto expected{f.integer<int16_t>()};if(actual[i]!=expected){std::cerr<<"sequence "<<sequence<<" event "<<events<<" field "<<i<<": "<<actual[i]<<" != "<<expected<<'\n';throw std::runtime_error{"native crew state"};}}
  };
  compare();auto count{f.integer<uint16_t>()};
  for(unsigned i{};i<count;++i){
   auto action{f.integer<int16_t>()};
   if(action==2003||action==2004)edit.cycle(h,c,action==2004?1:-1);
   else if(action==1623)edit.none(h,c);
   else if(action==1624)edit.select(h,c,static_cast<int16_t>((edit.selected_pilot()+1)%4));
   else if(action>=1615&&action<=1618){auto selected{static_cast<int16_t>(action-1615)};if(selected==edit.selected_pilot())edit.cycle(h,c,1);else edit.select(h,c,selected);}
   else if(action==100)edit.accept(h,c);
   else if(action==1700)edit.cancel(h,c);
   ++events;compare();
  }
 }
 f.require_end();std::cout<<cases<<" native crew-assignment sequences, "<<events<<" events agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
