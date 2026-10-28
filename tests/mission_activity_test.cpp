#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_activity.h"

namespace {
using namespace earthsiege;
std::vector<int16_t> words(binary_reader &file) {
  std::vector<int16_t> result(file.integer<uint16_t>()); for(auto &v:result)v=file.integer<int16_t>();return result;
}
random_state::state random_input(binary_reader &file) {
  random_state::state r;for(auto &v:r.words)v=file.integer<uint16_t>();r.first=file.integer<uint8_t>();r.second=file.integer<uint8_t>();return r;
}
void check_state(binary_reader &file, mission_activity &activity, random_state &random, bool const initial, std::string const &label) {
  for(auto const value:activity.variables()) if(value!=file.integer<int16_t>())file.fail(label+" counters");
  if(activity.entry_stagger()!=file.integer<uint16_t>())file.fail(label+" stagger");
  auto const rng{random_input(file)};
  if(initial) random=random_state{rng};
  else if(rng.words!=random.snapshot().words || rng.first!=random.snapshot().first || rng.second!=random.snapshot().second)file.fail(label+" RNG");
  auto const triggers{file.integer<uint16_t>()};
  for(size_t i{0};i!=triggers;++i)if(activity.triggered(i)!=(file.integer<uint8_t>()!=0))file.fail(label+" trigger");
  auto const waiting{words(file)};
  if(waiting.size()!=activity.all_groups().size())file.fail(label+" group count");
  for(size_t i{0};i!=waiting.size();++i)if(activity.group(i).activation_trigger!=(waiting[i]<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(waiting[i])}))file.fail(label+" activation");
  for(size_t i{0};i!=activity.size();++i){
    auto const &a{activity.actor(i).awareness};
    for(auto v:a.known)if(v!=file.integer<uint8_t>())file.fail(label+" known "+std::to_string(i));
    for(auto v:a.visible)if(v!=file.integer<uint8_t>())file.fail(label+" visible "+std::to_string(i));
    if(a.visibility_remaining!=file.integer<int16_t>() || a.refresh_remaining!=file.integer<int16_t>())file.fail(label+" clocks");
    if(a.seen!=(file.integer<uint8_t>()!=0) || a.alerted!=(file.integer<uint8_t>()!=0) || a.event_busy!=(file.integer<uint8_t>()!=0))file.fail(label+" contact flags");
  }
}
}
int main(int const argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMACT01")file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()};size_t total{0};
  for(size_t w{0};w!=worlds;++w){
    auto const demo{file.integer<uint16_t>()},count{file.integer<uint16_t>()},ng{file.integer<uint16_t>()};
    auto const name{"DEMO"+std::to_string(demo)};
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/(name+".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    mission_activity activity{script,placement_catalogue{resources}.place(script,roster),counters};random_state random;
    if(activity.size()!=count || activity.all_groups().size()!=ng)file.fail(name+" roster sizes");
    for(size_t i{0};i!=count;++i){
      auto const &a{activity.actor(i)};auto const label{name+" actor "+std::to_string(i)};
      if(static_cast<int16_t>(combat_kind(a.kind))!=file.integer<int16_t>() || a.group!=file.integer<uint16_t>() || a.member!=file.integer<uint16_t>())file.fail(label+" identity");
      if(a.player!=(file.integer<uint8_t>()!=0) || a.persistent_signature!=(file.integer<uint8_t>()!=0))file.fail(label+" flags");
      for(auto const ref:{a.contact_trigger,a.hit_trigger})if((ref?static_cast<int>(*ref):-1)!=file.integer<int16_t>())file.fail(label+" trigger");
      if(a.counter!=file.integer<int16_t>() || a.counter_operation!=file.integer<int16_t>())file.fail(label+" counter");
      auto const &c{activity.decisions()[i].controller};
      if((c.type?static_cast<int>(*c.type):-1)!=file.integer<int16_t>() || c.reconsider_remaining!=file.integer<int32_t>() || c.first_entry!=(file.integer<uint8_t>()!=0))file.fail(label+" controller");
    }
    for(size_t i{0};i!=ng;++i){
      auto const &g{activity.group(i)};
      if(static_cast<int16_t>(g.kind)!=file.integer<int16_t>() || g.order!=file.integer<int16_t>() || g.formation!=file.integer<int16_t>() || g.route_cursor!=file.integer<int16_t>() || g.side!=file.integer<uint8_t>())file.fail(name+" group identity");
      auto const members{words(file)},targets{words(file)};
      if(!std::ranges::equal(g.members,members) || !std::ranges::equal(g.objectives,targets))file.fail(name+" group links");
    }
    check_state(file,activity,random,true,name+" start");
    auto const steps{file.integer<uint16_t>()};
    for(size_t step{0};step!=steps;++step){
      auto const ratio{file.integer<int16_t>()};std::vector<observed_actor> inputs;
      for(size_t i{0};i!=count;++i)inputs.push_back({.position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()},.heading{file.integer<uint16_t>()},.dead{file.integer<uint8_t>()!=0},.disabled{file.integer<uint8_t>()!=0},.detectable{file.integer<uint8_t>()!=0}});
      std::vector<std::tuple<uint8_t,size_t,int>> expected,events;auto const ne{file.integer<uint16_t>()};
      for(size_t i{0};i!=ne;++i){auto const kind{file.integer<uint8_t>()};auto const first{file.integer<uint16_t>()};auto const second{file.integer<int16_t>()};expected.emplace_back(kind,first,second);}
      activity.dispatch_groups([&](size_t i){events.emplace_back(0,i,-1);});activity.scan_areas(inputs);
      activity.perceive(inputs,ratio,random,[&](size_t i,size_t j){events.emplace_back(1,i,static_cast<int>(j));return (i*7+j*3+step)%4!=0;});
      auto const label{name+" step "+std::to_string(step)};
      if(events!=expected)file.fail(label+" callback order");
      check_state(file,activity,random,false,label);++total;
    }
  }
  file.require_end();if(total!=128)file.fail("activity coverage");
  std::cout<<total<<" original mission activity passes agree across all four recorded starts\n";return EXIT_SUCCESS;
}catch(std::exception const &error){std::cerr<<error.what()<<'\n';return EXIT_FAILURE;}
