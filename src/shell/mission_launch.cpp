#include "mission_launch.h"
#include "simulation/fixed_math.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
prepared_mission prepare_mission(bytes source,int16_t mode,int16_t node,int16_t step,
  mission_options const &options,mission_counters const &history,random_state &random) {
  if((mode!=0&&mode!=1)||(mode&&(node<0||node>=8)))throw std::runtime_error{"invalid launch context"};
  prepared_mission out;if(mode)out.counters=history;
  out.counters[1]=mode?node:99;out.counters[2]=mode?step:0;out.counters[3]=options.time;
  for(size_t i{4};i<7;++i)out.counters[i]=random.bounded(12);
  out.compiled=compile_mission(source,out.counters,random,mode);auto &h{out.compiled.script.header};
  if(mode){constexpr std::array<terrain_kind,8> terrain{terrain_kind::temperate,terrain_kind::ice,terrain_kind::ice,terrain_kind::desert,terrain_kind::desert,terrain_kind::urban,terrain_kind::urban,terrain_kind::urban};
    h.terrain=terrain[node];h.unlimited_weapons=h.invulnerable_player=0;h.difficulty=2;
  }else{h.unlimited_weapons=options.unlimited;h.invulnerable_player=options.invulnerable;h.difficulty=options.difficulty;h.world_variant=options.environment;h.palette_variant=options.time;h.terrain=static_cast<terrain_kind>(random.bounded(4));}
  return out;
}
single_mission_crew generate_single_crew(resource_store const &resources,mission_script const &script,random_state &random) {
  single_mission_crew out;out.roster=read_hangar(resources.read("shell0","car/sm_init.dat"));for(auto &h:out.roster.hercs)normalize_hangar_equipment(h);auto &s{out.state};s.screen=1700;s.crew_count=4;
  s.technology=read_technology(resources.read("zones","bst/tech_sng.dat"));for(auto &c:s.crew)c=make_crew_member();
  if(script.groups.empty())throw std::runtime_error{"single mission has no player group"};
  auto const &members{script.groups.front().members};
  for(size_t i{};i<members.size()&&members[i]!=-1;++i){
    if(i>=s.crew.size())throw std::runtime_error{"single mission exceeds four crew"};
    auto &c{s.crew[i]};auto const &spawn{script.hercs.at(static_cast<size_t>(members[i]))};
    if(i==0){c.name=c.variant=0;c.hangar_slot=spawn.chassis;continue;}
    c.name=static_cast<int16_t>(random.bounded(4)+static_cast<int>(i-1)*4);c.variant=random.bounded(3);c.rank=random.bounded(6);
    constexpr std::array<int16_t,6> thresholds{0,1,5,15,30,-1};
    c.total[0]=static_cast<int16_t>(random.bounded(static_cast<int16_t>(thresholds[c.rank]+1))+thresholds[c.rank]);
    if(c.rank){c.total[1]=random.bounded(static_cast<int16_t>(c.rank*5));c.total[2]=random.bounded(static_cast<int16_t>(c.rank*10));}
    auto const base{out.roster.find(spawn.chassis)};if(!base)throw std::runtime_error{"single-mission chassis has no starting loadout"};
    player_herc h;h.chassis=spawn.chassis;auto const count{base->weapons.size()};h.weapons.assign(spawn.weapons.begin(),spawn.weapons.begin()+static_cast<ptrdiff_t>(count));h.weapon_values.assign(spawn.ammunition.begin(),spawn.ammunition.begin()+static_cast<ptrdiff_t>(count));h.extra=spawn.skill;
    h.condition.armour.fill(100);h.condition.internal.fill(100);h.condition.equipment.fill(100);
    int16_t slot{};while(slot<out.roster.capacity&&out.roster.find(slot))++slot;if(slot==out.roster.capacity)throw std::runtime_error{"single-mission hangar is full"};
    h.slot=slot;c.hangar_slot=slot;normalize_hangar_equipment(h);out.roster.hercs.push_back(std::move(h));
  }
  out.roster.selected=s.crew[0].hangar_slot;return out;
}
player_roster launch_player_roster(career_state const &crew,hangar &roster,bool campaign,int16_t player_identity,random_state &random) {
  player_roster out;out.selected=crew.selected_crew;
  if(out.selected<0||out.selected>=4)throw std::runtime_error{"invalid selected crew"};
  auto const assigned{std::ranges::count_if(crew.crew,[](auto const &c){return c.active&&c.hangar_slot!=-1;})};
  auto index{static_cast<size_t>(crew.selected_crew)};
  for(int i{};i<assigned;++i){auto const &c{crew.crew[index]};auto source{roster.find(c.hangar_slot)};
    if(!c.active||!source)throw std::runtime_error{"selected launch crew has no HERC"};
    int16_t const portrait{index==static_cast<size_t>(crew.selected_crew)?(campaign?player_identity:random.bounded(11)):(index==0?int16_t{-1}:wrap_word(c.name*3+c.variant))};
    int16_t const field{campaign?c.rank:static_cast<int16_t>(index==static_cast<size_t>(crew.selected_crew)?0:index)};
    if(!campaign){source->condition.armour.fill(100);std::fill_n(source->condition.internal.begin(),9,100);for(size_t j{};j<source->weapons.size();++j)source->condition.equipment[j]=100;}
    // The original writer uses the selected pilot's health for every exported HERC.
    source->condition.internal[9]=campaign?crew.crew[crew.selected_crew].health:100;
    auto h{*source};h.slot=portrait;h.field=field;out.hercs.push_back(std::move(h));
    if(i+1<assigned)do{index=(index+1)%4;}while(!crew.crew[index].active||crew.crew[index].hangar_slot==-1);
  }
  for(size_t i{};i<out.flags.size();++i)out.flags[i]=crew.technology.entries[8+i].available;
  return out;
}
} // namespace earthsiege
