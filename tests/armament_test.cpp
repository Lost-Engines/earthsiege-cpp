#include "shell/armament.h"
#include "render/hangar_artwork.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
player_herc herc(binary_reader &f){
  player_herc h;h.chassis=f.integer<int16_t>();auto n{f.integer<int16_t>()};
  for(int i{};i<10;++i){auto w{f.integer<int16_t>()};if(i<n)h.weapons.push_back(w);}
  for(int i{};i<10;++i){auto w{f.integer<int16_t>()};if(i<n)h.weapon_values.push_back(w);}
  h.extra=f.integer<int16_t>();
  for(auto &v:h.condition.armour)v=f.integer<int16_t>();
  for(auto &v:h.condition.internal)v=f.integer<int16_t>();
  for(auto &v:h.condition.equipment)v=f.integer<int16_t>();
  return h;
}
armory inventory(binary_reader &f){
  armory result;for(auto &stack:result){auto n{f.integer<uint16_t>()};while(n--)stack.push_back(f.integer<int16_t>());}return result;
}
bool equal(player_herc const &a,player_herc const &b){
  return a.chassis==b.chassis&&a.weapons==b.weapons&&a.weapon_values==b.weapon_values&&a.extra==b.extra
    &&a.condition.armour==b.condition.armour&&a.condition.internal==b.condition.internal&&a.condition.equipment==b.condition.equipment;
}
int main(int argc,char **argv)try{
  if(argc!=3)return 1;
  resource_store resources{argv[1]};armament_catalog catalog{resources};
  auto const data{read_file(argv[2])};binary_reader file{data};auto magic{file.take(8)};
  require(std::string(magic.begin(),magic.end())=="ESARM003","fixture version");
  auto transactions{file.integer<uint32_t>()},menus{file.integer<uint32_t>()},missiles{file.integer<uint32_t>()},cycles{file.integer<uint32_t>()};
  for(unsigned k{};k<transactions;++k){
    auto chassis{file.integer<int16_t>()},mount{file.integer<int16_t>()},weapon{file.integer<int16_t>()};
    auto h{herc(file)};auto stock{inventory(file)};auto expected{herc(file)};auto expected_stock{inventory(file)};
    require(h.chassis==chassis,"fixture chassis");catalog.exchange(h,mount,weapon,stock);
    require(equal(h,expected),"native equipment exchange mismatch");require(stock==expected_stock,"native armory exchange mismatch");
  }
  for(unsigned k{};k<menus;++k){
    auto chassis{file.integer<int16_t>()},mount{file.integer<int16_t>()},mode{file.integer<int16_t>()},current{file.integer<int16_t>()};(void)mode;
    technology_state technology;for(size_t j{};j<17;++j)technology.entries[8+j].available=file.integer<uint8_t>();
    auto stock{inventory(file)};auto n{file.integer<uint16_t>()};std::vector<int16_t> expected;while(n--)expected.push_back(file.integer<int16_t>());
    player_herc h;h.chassis=chassis;h.weapons.resize(catalog.mounts(chassis).size(),17);h.weapons.at(static_cast<size_t>(mount))=current;
    require(catalog.choices(h,mount,technology,stock)==expected,"native mount choices mismatch");
  }
  for(unsigned k{};k<missiles;++k){
    auto chassis{file.integer<int16_t>()},mount{file.integer<int16_t>()},type{file.integer<int16_t>()};
    player_herc h;h.weapon_values.resize(catalog.mounts(chassis).size(),5);select_missile_type(h,mount,type);
    require(h.weapon_values.at(static_cast<size_t>(mount))==type,"native missile choice mismatch");
  }
  for(unsigned k{};k<cycles;++k){
    auto mask{file.integer<int16_t>()};hangar h;career_state c;
    h.selected=file.integer<int16_t>();c.crew_cursor=file.integer<int16_t>();h.construction_slot=file.integer<int16_t>();
    auto direction{file.integer<int16_t>()},single{file.integer<int16_t>()};
    for(auto &member:c.crew)member.hangar_slot=file.integer<int16_t>();
    auto selected{file.integer<int16_t>()},cursor{file.integer<int16_t>()};
    std::array<int16_t,4> after{};for(auto &slot:after)slot=file.integer<int16_t>();
    for(int16_t i{};i<8;++i)if(mask&(1<<i)){player_herc p;p.slot=i;h.hercs.push_back(p);}
    cycle_arming_herc(h,c,direction,single!=0);
    require(h.selected==selected&&c.crew_cursor==cursor,"native hangar selection mismatch");
    for(size_t i{};i<4;++i)require(c.crew[i].hangar_slot==after[i],"native hangar reassignment mismatch");
  }
  for(int16_t chassis{};chassis<8;++chassis){
    auto const &mounts{catalog.mounts(chassis)};require(mounts.size()==file.integer<uint16_t>(),"native mount count");
    for(size_t mount{};mount<mounts.size();++mount){
      auto const &choices{mounts[mount]};require(choices.size()==file.integer<uint16_t>(),"native mount choice count");
      for(auto const &a:choices){
        require(a.weapon==file.integer<int16_t>()&&a.image==file.integer<int16_t>()&&a.hit_x==file.integer<int16_t>()&&a.hit_y==file.integer<int16_t>()&&a.draw_x==file.integer<int16_t>()&&a.draw_y==file.integer<int16_t>(),"native equipment coordinates");
      }
      auto const &r{catalog.hotspots(chassis).at(mount)};
      require(r.left==file.integer<int32_t>()&&r.top==file.integer<int32_t>()&&r.right==file.integer<int32_t>()&&r.bottom==file.integer<int32_t>(),"native hotspot geometry");
      require(catalog.hit(chassis,(r.left+r.right)/2,(r.top+r.bottom)/2)==static_cast<int>(mount),"click selects original mount");
    }
    for(auto value:hangar_artwork::origin[static_cast<size_t>(chassis)])require(value==file.integer<int32_t>(),"native bay origin");
    for(auto value:hangar_artwork::order[static_cast<size_t>(chassis)])require(value==file.integer<int16_t>(),"native bay painter order");
  }
  file.require_end();std::cout<<transactions<<" native equipment exchanges, "<<menus<<" mount menus and "<<missiles<<" missile selections, "<<cycles<<" HERC cycles and all mount coordinates match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
