#include "new_career.h"
#include "resources/binary_writer.h"
#include <stdexcept>
namespace earthsiege {
technology_state read_technology(bytes source,game_edition edition,int16_t battle_set) {
  binary_reader file{source,"technology table"};technology_state out;
  out.edition=edition;out.battle_set=battle_set;
  if(edition==game_edition::expansion){out.chassis_slots=9;out.entries.resize(31);}
  auto block=[&](int expected_type,int limit){
    auto count{file.integer<int16_t>()};if(count<0||count>limit)file.fail("invalid technology count");
    for(int i{};i<count;++i){auto type{file.integer<int16_t>()},id{file.integer<int16_t>()},early{file.integer<int16_t>()},late{file.integer<int16_t>()};
      if(edition==game_edition::expansion&&type==0&&id==16)id=8;
      if(type<0||type>1||(expected_type>=0&&type!=expected_type)||id<0||id>=(type?(edition==game_edition::expansion?22:17):static_cast<int>(out.chassis_slots)))file.fail("invalid technology entry");
      out.entries[id+(type?out.chassis_slots:0)]={early,late,0};
    }
    return static_cast<size_t>(count);
  };
  if(edition==game_edition::expansion){out.weapon_count=block(1,22);out.chassis_count=block(0,9);}
  else block(-1,25);
  file.require_end();advance_technology(out,0,0,0);out.last_type=out.last_index=-1;return out;
}
crew_member make_crew_member(random_state *random) {
  crew_member c;c.name=c.variant=-1;c.active=1;
  if(random){c.skill=random->bounded(2);c.prior_skill=c.skill;c.experience=static_cast<int16_t>(random->bounded(5)+c.skill*5);
    c.total[0]=static_cast<int16_t>((random->bounded(3)+1)*c.experience);c.total[2]=static_cast<int16_t>((random->bounded(2)+1)*c.experience);c.total[1]=static_cast<int16_t>(random->bounded(2)*c.experience);
  }
  c.rank=pilot_rank(c.total[0]);return c;
}
std::vector<uint8_t> campaign_start::initial_career_file() const {
  // Avoid changing the runtime lists to manufacture the original initial file.
  binary_writer file;file.integer(career.active);file.integer(career.mode);for(auto name:career.names)file.integer(name);
  auto const inventory{write_armory(career.inventories[0])};
  for(auto const &state:career.states){file.append(write_career_state(state));file.append(inventory);}
  return std::move(file).finish();
}
campaign_start create_career(resource_store const &resources,int16_t battle_set,int16_t repair_policy,random_state &random,uint8_t difficulty) {
  campaign_start out;auto &career{out.career};career.edition=resources.edition();career.active=career.edition==game_edition::expansion?difficulty:1;career.mode=battle_set;
  auto const directory{career.edition==game_edition::expansion&&battle_set?"bar/":"car/"};
  for(int group{};group<3;++group){std::array<int16_t,4> permutation{-1,-1,-1,-1};
    for(int16_t value{};value<4;++value){auto remaining{random.bounded(static_cast<int16_t>(4-value))};size_t slot{};
      while(remaining){if(permutation[slot]==-1)--remaining;++slot;}
      while(permutation[slot]!=-1)++slot;
      permutation[slot]=value;
    }
    for(size_t i{};i<4;++i)career.names[group*4+i]=static_cast<int16_t>(group*4+permutation[i]);
  }
  binary_reader inventory{resources.read("shell0",std::string(directory)+"c_armory.dat"),"initial armory"};career.inventories[0]=read_armory(inventory);inventory.require_end();
  if(career.edition==game_edition::expansion&&battle_set)career.inventories[0].resize(22);
  auto const technology{read_technology(resources.read("zones","bst/tech_"+std::to_string(career.edition==game_edition::expansion?battle_set:0)+".dat"),career.edition,battle_set)};
  auto const initial_hangar{read_hangar(resources.read("shell0",std::string(directory)+"car_init.dat"),career.edition)};
  if(career.edition==game_edition::expansion)for(size_t i{1};i<career.inventories.size();++i)career.inventories[i]=armory{22};
  for(size_t i{};i<career.states.size();++i){auto &s{career.states[i]};s.screen=1700;s.crew_count=4;s.technology=technology;
    for(size_t j{};j<4;++j)s.crew[j]=make_crew_member(j?&random:nullptr);
    if(i==0){s.valid=1;s.budget=career.edition==game_edition::expansion&&battle_set?100000:60000;s.repair_policy=repair_policy;s.crew[0].hangar_slot=0;
      for(size_t j{1};j<4;++j){auto &c{s.crew[j]};c.name=career.names[(j-1)*4];c.variant=0;c.next_name=1;c.rotation=0;c.hangar_slot=static_cast<int16_t>(j);c.active=1;}
    }
    out.hangars[i]=initial_hangar;
  }
  return out;
}
} // namespace earthsiege
