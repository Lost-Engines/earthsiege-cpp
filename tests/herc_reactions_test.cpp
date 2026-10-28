#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/binary_reader.h"
#include "simulation/herc_reactions.h"
namespace {
std::optional<size_t> reference(int32_t v) {return v < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(v)};}
}
int main(int argc,char **argv) try {
  using namespace earthsiege;if(argc!=2)return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESHREA01")file.fail("fixture version");
  auto const read{[&]{return file.integer<int32_t>();}};auto const count{read()};
  for(int trial{0};trial!=count;++trial) {
    bool const player{read()!=0},same_group{read()!=0},same_side{read()!=0};auto const attacker{static_cast<size_t>(read())};
    bool const leader_player{read()!=0};auto const leader_target{reference(read())};auto const kind{static_cast<combat_actor_kind>(read())};
    auto const old{reference(read())},chosen{reference(read())};auto const type{static_cast<behavior>(read())};
    herc_reaction_state state{read(),read(),read(),read()};auto const damage{static_cast<int16_t>(read())};
    engagement_state decision;decision.command=static_cast<int16_t>(read());bool ignore_hud_target{read()!=0};auto designated{reference(read())};
    decision.controller.type=type;decision.controller.reconsider_remaining=read();
    size_t const observer{player?0U:1U};std::array<target_relationship,4> targets;
    for(auto &t:targets) t.attackers=10;
    targets[observer].selected=old;
    std::vector<std::pair<int,int>> events;
    react_to_herc_hit(state,decision,targets,observer,attacker,damage,
      {player,same_group,attacker==0,same_side,leader_player,leader_target==attacker,kind},ignore_hud_target,designated,{
      .share_contact{[&]{events.emplace_back(0,0);}}, .friendly_warning{[&]{events.emplace_back(1,8);}}, .under_attack{[&]{events.emplace_back(1,3);}},
      .select{[&]{events.emplace_back(2,0);return chosen;}}, .resume_orders{[&]{events.emplace_back(3,0);}}, .strategy{[&]{events.emplace_back(4,0);}},
    });
    auto const check{[&](char const *name,auto expected,auto actual){if(expected!=actual){std::cerr<<trial<<' '<<name<<": expected "<<expected<<", actual "<<actual<<'\n';file.fail(name);}}};
    auto const event_count{read()};check("event count",event_count,static_cast<int>(events.size()));
    for(auto const &[event_kind,value]:events){check("event",read(),event_kind);check("voice",read(),value);}
    check("cooldown",read(),state.retaliate_remaining);check("warning",read(),state.warning_remaining);check("damage timer",read(),state.damage_remaining);check("accumulated",read(),state.incoming_damage);
    check("selected",read(),targets[observer].selected?static_cast<int>(*targets[observer].selected):-1);
    for(auto const &t:targets)check("attackers",read(),t.attackers);
    check("changed",read()!=0,targets[observer].changed);check("requested",read()!=0,decision.attack_requested);check("ignore_hud_target",read()!=0,ignore_hud_target);
    check("command",read(),decision.command);check("designated",read(),designated?static_cast<int>(*designated):-1);check("clock",read(),decision.controller.reconsider_remaining);
    advance_herc_reactions(state,static_cast<frame_ratio>(read()));
    check("ticked cooldown",read(),state.retaliate_remaining);check("ticked warning",read(),state.warning_remaining);check("ticked damage timer",read(),state.damage_remaining);check("ticked damage",read(),state.incoming_damage);
  }
  file.require_end();if(count!=4096)file.fail("coverage");std::cout<<count<<" original HERC reactions and clock updates match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
