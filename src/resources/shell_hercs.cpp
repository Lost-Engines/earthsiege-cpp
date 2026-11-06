#include "shell_hercs.h"
namespace earthsiege {
std::vector<shell_herc> read_shell_hercs(resource_store const &r){
 std::vector<shell_herc> out;
 if(r.edition()==game_edition::expansion){
  auto raw{r.read("shell0","cfg/herc_res.dat")};binary_reader f{raw,"shell HERC resources"};auto count{f.integer<uint16_t>()};
  if(count!=9)f.fail("invalid shell HERC count");
  for(unsigned i{};i<count;++i){shell_herc h;h.chassis=f.integer<int16_t>();if(h.chassis!=human_herc_id(i))f.fail("invalid shell HERC identifier");
   auto mounts{f.integer<uint16_t>()};if(mounts>10)f.fail("invalid shell mount count");for(unsigned j{};j<mounts;++j)h.painter_order.push_back(f.integer<int16_t>());
   for(auto &name:h.artwork)name=f.cstring();h.origin={f.integer<int32_t>(),f.integer<int32_t>()};
   auto positions{f.integer<uint32_t>()};if(positions>3)f.fail("invalid construction stages");for(unsigned j{};j<positions;++j)h.construction_positions.push_back({f.integer<int32_t>(),f.integer<int32_t>()});
   h.loadout_diagram=f.cstring();h.damage_diagram=f.cstring();out.push_back(std::move(h));
  }f.require_end();return out;
 }
 constexpr std::array art{"road","guns","rapt","toma","patr","pnth","herc","coll"};
 constexpr std::array config{"road","guns","rapt","toma","patr","pant","herc","colo"};
 constexpr std::array damage{"road","gunn","rapt","toma","patr","pnth","herc","colo"};
 constexpr std::array<vec2<int32_t>,8> origins{{{77,99},{77,89},{83,64},{86,53},{75,57},{73,37},{72,37},{58,26}}};
 const std::array<std::vector<int16_t>,8> orders{{{1,2,0},{1,2,0},{3,2,1,0},{4,3,2,1,0},{5,2,4,1,3,0},{1,6,4,3,5,2,0},{1,6,4,3,7,5,2,0},{1,8,3,6,4,2,5,7,0}}};
 const std::array<std::vector<vec2<int32_t>>,8> positions{{{},{},{{84,97}},{{86,94}},{{84,92}},{{78,50},{78,85}},{{78,51},{78,91}},{{74,41},{74,82}}}};
 for(size_t i{};i<8;++i){shell_herc h;h.chassis=human_herc_id(i);h.painter_order=orders[i];h.origin=origins[i];h.construction_positions=positions[i];
  h.artwork={"res32/"+std::string(art[i])+"wep1.dba","res32/"+std::string(art[i])+"wepd.dba","res32/"+std::string(art[i])+"outl.dba","res32/"+std::string(damage[i])+"dmg1.dba",i<2?"":"res32/"+std::string(i==7?"colo":art[i])+"bild.dba"};
  h.loadout_diagram="cfg/"+std::string(config[i])+".dat";h.damage_diagram="cfg/"+std::string(damage[i])+"dmg1.dat";out.push_back(std::move(h));
 }return out;
}
}
