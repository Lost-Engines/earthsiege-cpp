#include "career_transfer.h"
#include <stdexcept>
namespace earthsiege {
campaign_start transfer_career(resource_store const &resources,campaign_start const &source,bool keep,random_state &random){
 if(resources.edition()!=game_edition::expansion||source.career.mode!=0)throw std::runtime_error{"career transfer requires an original battleset career and expansion data"};
 campaign_start target{source};auto &c{target.career};c.edition=game_edition::expansion;c.mode=1;
 auto const technology{read_technology(resources.read("zones","bst/tech_1.dat"),game_edition::expansion,1)};
 std::optional<campaign_start> fresh;
 if(!keep){fresh=create_career(resources,1,source.career.states[0].repair_policy,random,source.career.active);c.names=fresh->career.names;}
 for(size_t i{};i<c.states.size();++i){auto &s{c.states[i]};
  s.valid=static_cast<uint8_t>(i==0);s.current=0;s.node_progress=0;s.screen=1700;s.node=0;s.step=0;s.missions=0;s.selected_crew=0;s.crew_cursor=0;s.crew_count=4;s.counters.fill(0);s.technology=technology;
  if(fresh){s.crew=fresh->career.states[i].crew;s.crew[0].total=source.career.states[i].crew[0].total;s.crew[0].rank=pilot_rank(s.crew[0].total[0]);}
  if(i==0){s.budget=keep?source.career.states[0].budget:100000;s.repair_policy=source.career.states[0].repair_policy;}
  c.inventories[i]=keep?source.career.inventories[0]:fresh->career.inventories[0];c.inventories[i].resize(22);
  target.hangars[i]=keep?source.hangars[0]:fresh->hangars[0];
 }
 return target;
}
}
