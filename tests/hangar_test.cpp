#include "shell/hangar_service.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
static void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
static herc_condition condition(binary_reader &f){herc_condition c;for(auto &v:c.armour)v=f.integer<int16_t>();for(auto &v:c.internal)v=f.integer<int16_t>();for(auto &v:c.equipment)v=f.integer<int16_t>();return c;}
static bool equal(herc_condition const &a,herc_condition const &b){return a.armour==b.armour&&a.internal==b.internal&&a.equipment==b.equipment;}
static void weapons(binary_reader &f,player_herc &h){auto const n{f.integer<uint16_t>()};for(unsigned j{};j<n;++j)h.weapons.push_back(f.integer<int16_t>());}
int main(int argc,char **argv)try {
  if(argc!=3)return 1;
  resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader file{data};
  auto const m{file.take(8)};require(std::string(m.begin(),m.end())=="ESHGR001","fixture version");
  auto const prices{read_service_prices(resources.read("shell0","cfg/damage.dat"))};
  for(auto const &c:prices.chassis){require(c.purchase==file.integer<int32_t>()&&c.construction_missions==file.integer<int16_t>(),"purchase/duration mismatch");for(auto v:c.armour)require(v==file.integer<int16_t>(),"armor price mismatch");for(auto v:c.internal)require(v==file.integer<int16_t>(),"internal price mismatch");}
  for(auto v:prices.weapons)require(v==file.integer<int16_t>(),"weapon price mismatch");
  auto const repairs{file.integer<uint32_t>()};
  for(unsigned i{};i<repairs;++i){player_herc h;h.chassis=file.integer<int16_t>();weapons(file,h);h.condition=condition(file);
    for(int16_t target:std::array<int16_t,5>{100,90,74,51,0})require(repair_quote(h,target,prices)==file.integer<int32_t>(),"native repair quote mismatch");
    auto const budget{file.integer<int32_t>()};auto const expected{condition(file)};auto const total{file.integer<int32_t>()},share{file.integer<int32_t>()};
    auto const result{repair_herc(h,budget,prices)};
    require(equal(h.condition,expected),"native repaired conditions mismatch");
    require(1000000-result.cost==total&&(result.applied?budget-result.cost:12345)==share,"native repair transaction mismatch");
  }
  auto const construction{file.integer<uint32_t>()};
  for(unsigned i{};i<construction;++i){hangar roster;player_herc h;h.chassis=file.integer<int16_t>();h.slot=h.chassis;roster.construction_slot=h.slot;roster.construction_remaining=file.integer<int16_t>();weapons(file,h);h.condition=condition(file);
    auto const expected{condition(file)};auto const slot{file.integer<int16_t>()},remaining{file.integer<int16_t>()};auto const value{file.integer<int32_t>()};roster.hercs.push_back(h);
    advance_construction(roster,prices);require(equal(roster.hercs[0].condition,expected)&&roster.construction_slot==slot&&roster.construction_remaining==remaining&&roster.construction_value==value,"native construction mismatch");
  }
  file.require_end();std::cout<<repairs*5<<" native quotes, "<<repairs<<" repair transactions and "<<construction<<" construction states match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
