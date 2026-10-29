#include "shell/hangar_service.h"
#include "shell/armament.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
player_herc read_native(binary_reader &f){
 player_herc h;h.chassis=f.integer<int16_t>();auto n{f.integer<int16_t>()};
 for(int i{};i<10;++i){auto v{f.integer<int16_t>()};if(i<n)h.weapons.push_back(v);}
 for(int i{};i<10;++i){auto v{f.integer<int16_t>()};if(i<n)h.weapon_values.push_back(v);}
 h.extra=f.integer<int16_t>();for(auto &v:h.condition.armour)v=f.integer<int16_t>();for(auto &v:h.condition.internal)v=f.integer<int16_t>();for(auto &v:h.condition.equipment)v=f.integer<int16_t>();return h;
}
bool equal(player_herc const &a,player_herc const &b){return a.chassis==b.chassis&&a.weapons==b.weapons&&a.weapon_values==b.weapon_values&&a.extra==b.extra&&a.condition.armour==b.condition.armour&&a.condition.internal==b.condition.internal&&a.condition.equipment==b.condition.equipment;}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};armament_catalog catalog{r};auto prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};require(std::string(magic.begin(),magic.end())=="ESHSRV01","fixture version");
 auto builds{f.integer<uint32_t>()},scraps{f.integer<uint32_t>()};
 for(unsigned i{};i<builds;++i){
  auto chassis{f.integer<int16_t>()};auto budget{f.integer<int32_t>()};auto remaining{f.integer<int16_t>()};auto value{f.integer<int32_t>()};auto expected{read_native(f)};
  hangar h;career_state c;c.budget=1000000;require(reserve_construction(h)==0,"first empty construction slot");
  require(construct_herc(h,c,chassis,catalog.mounts(chassis).size(),prices),"construct");
  require(c.budget==budget&&h.construction_remaining==remaining&&h.construction_value==value,"construction cost and time");
  require(equal(*h.find(0),expected),"bare construction model");
 }
 for(unsigned i{};i<scraps;++i){
  auto mask{f.integer<int16_t>()};hangar h;career_state c;armory stocks;for(auto &s:stocks)s={88,72};c.budget=123456;
  h.selected=f.integer<int16_t>();h.construction_slot=f.integer<int16_t>();h.construction_remaining=f.integer<int16_t>();h.construction_value=f.integer<int32_t>();
  for(auto &m:c.crew)m.hangar_slot=f.integer<int16_t>();
  auto selected{read_native(f)};selected.slot=h.selected;
  for(int16_t j{};j<8;++j)if(mask&(1<<j)){player_herc model;model.slot=j;h.hercs.push_back(j==selected.slot?selected:model);}
  auto count{f.integer<int16_t>()},slot{f.integer<int16_t>()},construction{f.integer<int16_t>()},remaining{f.integer<int16_t>()};
  auto value{f.integer<int32_t>()},budget{f.integer<int32_t>()};auto cursor{f.integer<int16_t>()};std::array<int16_t,4> crew;for(auto &v:crew)v=f.integer<int16_t>();
  armory expected;for(auto &s:expected){auto n{f.integer<uint16_t>()};while(n--)s.push_back(f.integer<int16_t>());}
  require(scrap_selected(h,c,stocks,prices),"scrap");
  require(h.hercs.size()==static_cast<size_t>(count)&&h.selected==slot&&h.construction_slot==construction&&h.construction_remaining==remaining&&h.construction_value==value,"scrap roster");
  require(c.budget==budget&&c.crew_cursor==cursor,"scrap budget and cursor");
  for(size_t j{};j<crew.size();++j)require(c.crew[j].hangar_slot==crew[j],"scrap assignment");
  require(stocks==expected,"salvaged equipment order");
 }
 f.require_end();std::cout<<builds<<" native constructions and "<<scraps<<" scrap transactions agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
