#include "armament.h"
#include "resources/shell_hercs.h"
#include <algorithm>
namespace earthsiege {
armament_catalog::armament_catalog(resource_store const &resources) {
  auto const hercs{read_shell_hercs(resources)};chassis_.resize(hercs.size());hotspots_.resize(hercs.size());
  for(size_t i{};i<hercs.size();++i){
    auto const data{resources.read("shell0",hercs[i].loadout_diagram)};
    binary_reader file{data,"shell weapon mounts"};auto const n{file.integer<uint16_t>()};
    if(n>10)file.fail("invalid mount count");
    for(unsigned mount{};mount<n;++mount){
      auto const count{file.integer<uint16_t>()};if(count==0||count>22)file.fail("invalid mount choices");
      std::vector<mount_artwork> choices;
      for(unsigned j{};j<count;++j){
        mount_artwork a;a.image=file.integer<int16_t>();a.weapon=file.integer<int16_t>();
        a.hit_x=file.integer<int16_t>();a.hit_y=file.integer<int16_t>();a.draw_x=file.integer<int16_t>();a.draw_y=file.integer<int16_t>();
        if(a.weapon<0||static_cast<size_t>(a.weapon)>=std::max<size_t>(18,weapon_count(resources.edition()))||a.image<0)file.fail("invalid equipment reference");
        choices.push_back(a);
      }
      if(choices.front().weapon!=17)file.fail("mount must start with empty equipment");
      chassis_[i].push_back(std::move(choices));
    }
    file.require_end();
  }
  auto const data{resources.read("shell0","cfg/hotspot.dat")};binary_reader file{data,"hangar selection regions"};
  if(file.integer<uint16_t>()!=hercs.size())file.fail("invalid hotspot chassis count");
  for(auto &list:hotspots_){
    auto const count{file.integer<uint16_t>()};if(count>10)file.fail("invalid hotspot count");
    for(unsigned j{};j<count;++j){
      auto x0{file.integer<uint16_t>()},y0{file.integer<uint16_t>()},x1{file.integer<uint16_t>()},y1{file.integer<uint16_t>()};
      list.push_back({std::min(x0,x1),std::min(y0,y1),std::max(x0,x1),std::max(y0,y1)});
    }
  }
  file.require_end();
}
std::optional<int> armament_catalog::hit(int16_t chassis,int x,int y)const {
  auto const &list{hotspots(chassis)};
  for(size_t i{};i<list.size();++i)if(list[i].contains(x,y))return static_cast<int>(i);
  return {};
}
mount_artwork const *armament_catalog::find(int16_t chassis,int mount,int16_t weapon)const {
  auto const &all{mounts(chassis)};if(mount<0||static_cast<size_t>(mount)>=all.size())return nullptr;
  auto const &list{all[static_cast<size_t>(mount)]};
  auto it{std::ranges::find(list,weapon,&mount_artwork::weapon)};return it==list.end()?nullptr:&*it;
}
std::vector<int16_t> armament_catalog::choices(player_herc const &herc,int mount,
  technology_state const &technology,armory const &inventory)const {
  auto available{[&](int16_t w){return static_cast<size_t>(w)<inventory.size()&&(technology.weapon_available(static_cast<size_t>(w))||!inventory.at(static_cast<size_t>(w)).empty());}};
  std::vector<int16_t> result;
  if(mount<0){for(int16_t w{};static_cast<size_t>(w)<inventory.size();++w)if(w!=17&&available(w))result.push_back(w);return result;}
  result.push_back(17);auto const &list{mounts(herc.chassis).at(static_cast<size_t>(mount))};
  for(auto const &item:list)if(item.weapon!=17&&(available(item.weapon)||herc.weapons.at(static_cast<size_t>(mount))==item.weapon))result.push_back(item.weapon);
  return result;
}
bool armament_catalog::exchange(player_herc &herc,int mount,int16_t weapon,armory &inventory)const {
  if(!find(herc.chassis,mount,weapon))return false;
  auto const i{static_cast<size_t>(mount)};auto const old{herc.weapons.at(i)};
  if(old==weapon)return false;
  int16_t condition{100};
  if(weapon!=17){
    auto &stack{inventory.at(static_cast<size_t>(weapon))};if(stack.empty())return false;
    condition=stack.front();stack.erase(stack.begin());
  }
  if(old!=17){auto &stack{inventory.at(static_cast<size_t>(old))};stack.insert(stack.begin(),herc.condition.equipment.at(i));}
  herc.condition.equipment.at(i)=condition;herc.weapons.at(i)=condition?weapon:17;
  auto &value{herc.weapon_values.at(i)};
  if(herc.weapons[i]<9||herc.weapons[i]>11)value=5;else if(value==5)value=0;
  return true;
}
void cycle_arming_herc(hangar &roster,career_state &crew,int direction,bool single) {
  if(roster.capacity<=0)return;
  auto next{roster.selected};
  for(int i{};i<roster.capacity;++i){
    if(direction>0)next=static_cast<int16_t>((next+1)%roster.capacity);
    else{if(next<1)next=roster.capacity;--next;}
    if(!roster.find(next)||next==roster.construction_slot)continue;
    if(single&&std::ranges::any_of(crew.crew,[&](auto const &c){return c.hangar_slot==next;}))continue;
    roster.selected=next;break;
  }
  if(single){if(crew.crew_cursor>=0&&crew.crew_cursor<4)crew.crew[static_cast<size_t>(crew.crew_cursor)].hangar_slot=roster.selected;}
  else{
    crew.crew_cursor=-1;
    for(size_t i{};i<crew.crew.size();++i)if(crew.crew[i].hangar_slot==roster.selected){crew.crew_cursor=static_cast<int16_t>(i);break;}
  }
}
void select_missile_type(player_herc &herc,int mount,int16_t type) {
  if(mount>=0)herc.weapon_values.at(static_cast<size_t>(mount))=type;
}
} // namespace earthsiege
