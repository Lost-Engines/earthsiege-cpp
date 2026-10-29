#include "game_state.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
game_state::game_state(resource_store const &data,int seed):resources{data},random{static_cast<int16_t>(seed)}{}
career_state &game_state::crew(){return campaign_?career_->career.states[0]:single_.state;}
career_state const &game_state::crew()const{return campaign_?career_->career.states[0]:single_.state;}
armory &game_state::inventory(){return campaign_?career_->career.inventories[0]:single_.inventory;}
armory const &game_state::inventory()const{return campaign_?career_->career.inventories[0]:single_.inventory;}
hangar &game_state::roster(){return campaign_?career_->hangars[0]:single_.roster;}
hangar const &game_state::roster()const{return campaign_?career_->hangars[0]:single_.roster;}
void game_state::new_campaign(int16_t battle,int16_t policy,int16_t identity){
  career_=create_career(resources,battle,policy,random);prepared_.reset();identity_=identity;campaign_=true;last_grade_=last_kind_=0;
  prepare_campaign();
}
void game_state::load_campaign(campaign_start saved,int16_t identity){
  career_=std::move(saved);identity_=identity;campaign_=true;prepared_.reset();
  auto const &s{crew()};last_grade_=s.counters[0];last_kind_=s.mission_kind;
  for(auto &h:roster().hercs)normalize_hangar_equipment(h);
}
void game_state::prepare_single(int16_t category,mission_options const &options){
  campaign_=false;auto const groups{read_single_missions(resources.read("zones","msn/sing_msn.dat"))};
  if(category==-1)category=random.bounded(static_cast<int16_t>(groups.size()));
  auto const &choices{groups.at(static_cast<size_t>(category))};
  prepare(choices.at(static_cast<size_t>(random.bounded(static_cast<int16_t>(choices.size())))),options);
}
void game_state::prepare_campaign(bool next,mission_options const &options){
  if(!career_)throw std::runtime_error{"no selected career"};
  campaign_=true;
  auto &c{career_->career};auto &s{c.states[0]};
  auto const table{read_campaign_table(resources.read("zones","bst/btlset_"+std::to_string(c.mode)+".dat"))};
  bool const branch{campaign_failure_branch(last_kind_,last_grade_)};
  if(next){c.active=0;s.current=0;if(table.advance(s.node,s.step,branch)){s.node_progress=0;s.counters.fill(0);}}
  if(s.node==-1||s.node==99){c.active=0;prepared_.reset();return;}
  prepare(table.filename(s.node,s.step,branch),options);
  // GO 0x15cbb: stable descending rank of the three wingmates. The player is
  // left in place; the mission's authored first group sets the active count.
  auto const &members{prepared_->compiled.script.groups.at(0).members};
  auto const count{std::ranges::find(members,int16_t{-1})-members.begin()};
  std::array<size_t,3> order{1,2,3};
  std::stable_sort(order.begin(),order.end(),[&s](size_t a,size_t b){return s.crew[a].rank>s.crew[b].rank;});
  for(size_t i{};i<3;++i)s.crew[order[i]].active=static_cast<uint8_t>(static_cast<ptrdiff_t>(i+1)<count);
  s.mission_kind=prepared_->compiled.script.header.objective_kind;s.screen=1700;
}
void game_state::prepare(std::string name,mission_options const &options){
  mission_name_=std::move(name);if(mission_name_.ends_with(".MSN")||mission_name_.ends_with(".msn"))mission_name_.resize(mission_name_.size()-4);
  auto const &s{crew()};
  auto const history{campaign_ && prepared_ ? prepared_->counters : s.counters};
  prepared_=prepare_mission(resources.read("zones","msn/"+mission_name_+".msn"),campaign_?1:0,s.node,s.step,options,history,random);
  text_=read_mission_text(resources.read("zones","msn/"+mission_name_+".eng"));
  if(!campaign_)single_=generate_single_crew(resources,prepared_->compiled.script,random);
  // A saved career may temporarily have no player HERC. The service bay must
  // remain usable so its original warning and crew screen can resolve this.
  auto const pilot{crew().selected_crew};
  if(pilot>=0&&pilot<4&&crew().crew[static_cast<size_t>(pilot)].active&&roster().find(crew().crew[static_cast<size_t>(pilot)].hangar_slot))update_roster();
  else player_={};
  if(campaign_)checkpoint();
}
void game_state::checkpoint(size_t history) {
  if(!campaign_||!career_)return;
  auto &s{crew()};
  checkpoint_career(career_->career,history,s.node_progress,s.budget,prepared_?prepared_->counters:s.counters,s.repair_policy);
  if(history)career_->hangars.at(history)=roster();
}
void game_state::update_roster(){player_=launch_player_roster(crew(),roster(),campaign_,identity_,random);}
std::unique_ptr<mission_session> game_state::launch(){
  if(!prepared_)throw std::runtime_error{"mission has not been prepared"};
  update_roster();
  return std::make_unique<mission_session>(resources,prepared_->compiled.script,player_,random_state{},prepared_->counters);
}
void game_state::restore_history(size_t history){
  if(!career_||history==0||history>=41||!career_->career.states[history].valid)throw std::runtime_error{"invalid career history"};
  auto &c{career_->career};c.states[0]=c.states[history];c.inventories[0]=c.inventories[history];career_->hangars[0]=career_->hangars[history];
  for(auto &stack:c.inventories[0])std::ranges::reverse(stack);
  last_grade_=c.states[0].counters[0];last_kind_=c.states[0].mission_kind;prepared_.reset();
}
void game_state::resume_shell(int16_t seed,std::optional<campaign_start> saved){
  random=random_state{seed};
  if(campaign_){
    if(!saved)throw std::runtime_error{"campaign return needs the launch save"};
    career_=std::move(*saved);
    for(auto &h:roster().hercs)normalize_hangar_equipment(h);
    roster().selected=crew().crew.at(static_cast<size_t>(crew().selected_crew)).hangar_slot;
  }
}
void game_state::complete(mission_results const &result,mission_counters const &counters,std::function<void()> const &before_repair){
  if(!prepared_)throw std::runtime_error{"no mission to return from"};
  last_grade_=result.grade;last_kind_=prepared_->compiled.script.header.objective_kind;
  prepared_->counters=counters;prepared_->counters[0]=result.grade;
  if(!campaign_){single_.state.counters=prepared_->counters;if(before_repair)before_repair();return;}
  auto &c{career_->career};auto &s{c.states[0]};auto &h{career_->hangars[0]};
  auto const prices{read_service_prices(resources.read("shell0","cfg/damage.dat"))};
  auto const old_technology{s.technology.entries};
  auto const debrief{return_from_mission(c,h,result,counters,prices,random)};
  returned_={};returned_.recovery=result.recovery;returned_.deaths=debrief.deaths;
  returned_.alive=s.crew.at(static_cast<size_t>(s.selected_crew)).health>0;
  if(auto player{h.find(s.crew[0].hangar_slot)})returned_.condition=herc_average_condition(*player);
  for(size_t i{};i<old_technology.size();++i)returned_.technology|=!old_technology[i].available&&s.technology.entries[i].available;
  auto const table{read_campaign_table(resources.read("zones","bst/btlset_"+std::to_string(c.mode)+".dat"))};
  auto next_node{s.node},next_step{s.step};
  if(table.advance(next_node,next_step,campaign_failure_branch(last_kind_,last_grade_)))returned_.continuation=next_node;

  if(before_repair)before_repair();
  apply_repair_policy(s,h,prices);
  // GO's return dispatcher saves only the working state. Backups are explicit
  // actions in the Career Backups screen (0x167cd), not automatic snapshots.
  checkpoint_career(c,0,s.node_progress,s.budget,s.counters,s.repair_policy);
  prepared_->counters=s.counters;
}
}
