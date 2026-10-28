#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include "resources/binary_reader.h"
#include "simulation/mission_objectives.h"

namespace {
struct fixture {
  std::array<earthsiege::objective_actor,3> actors{};
  std::array<earthsiege::objective_group,3> groups{};
  std::array<std::array<size_t,1>,3> members{{{{0}},{{1}},{{2}}}};
  std::array<std::array<uint8_t,3>,3> known{};
  earthsiege::objective_world world;
  fixture():world{actors,groups,0,{{-1000,-2000},{3000,4000}}} {
    for(size_t i{0};i!=3;++i) {actors[i].group=i;actors[i].known=known[i];groups[i].members=members[i];}
    groups[0].target_actor=1;groups[1].side=1;groups[2].side=1;
  }
  void danger(bool enabled) {known[2][0]=static_cast<uint8_t>(enabled);actors[2].position=actors[0].position;}
};
}
int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader f{bytes};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMOBJ01") f.fail("fixture version");
  auto const read{[&]{return f.integer<int32_t>();}};auto const word{[&]{return static_cast<int16_t>(read());}};
  auto const point{[&]{return vec3<int32_t>{read(),read(),read()};}};
  int trial{0};
  auto const check{[&](char const *label,auto expected,auto actual){if(expected!=actual) {std::cerr<<trial<<' '<<label<<": expected "<<expected<<", actual "<<actual<<'\n';f.fail(label);}}};
  mission_outcome_clock clock;
  auto const count{read()};
  for(;trial!=count;++trial) {
    fixture v;auto const type{read()};int actual{0};
    switch(type) {
      case 0: v.actors[1].damage_fraction=word();v.actors[1].dead=read()!=0;v.actors[1].removed=read()!=0;actual=v.world.actor_grade(1);break;
      case 1: {
        auto const n{read()};std::vector<objective_actor> actors(static_cast<size_t>(n));std::vector<size_t> members;
        for(int i{0};i!=n;++i) {actors[static_cast<size_t>(i)].damage_fraction=word();members.push_back(static_cast<size_t>(i));}
        for(auto &actor:actors) actor.disabled=read()!=0;
        std::array<objective_group,1> groups;groups[0].members=members;
        actual=objective_world{actors,groups,0,{}}.group_grade(0);break;
      }
      case 2: {auto const margin{read()};auto const pos{point()};actual=v.world.bounds.outside(pos,margin);break;}
      case 3: {
        v.actors[2].position.x=read();bool const targeting{read()!=0};v.known[2][0]=static_cast<uint8_t>(read());auto const flag{read()};
        v.groups[2].waiting=read()!=0;v.groups[2].side=read()!=0 ? 0 : 1;
        v.actors[2].dead=flag==0x93;v.actors[2].disabled=flag==0x9e;v.actors[2].unavailable=flag==0x9f;
        if(targeting) v.actors[2].selected=0;
        actual=v.world.safe(0);break;
      }
      case 4: {
        auto const kind{word()};bool const complete{read()!=0},danger{read()!=0};auto const boundary{read()};bool const request{read()!=0};
        v.actors[0].arrived=complete;v.actors[0].rescued=complete;v.groups[0].route_complete=complete;v.groups[1].route_complete=complete;
        v.groups[1].waiting=!complete;v.actors[1].dead=complete && (kind==6 || kind==8 || kind==9);
        v.actors[0].position.x=boundary==0 ? 0 : boundary==1 ? 63000 : 173000;v.danger(danger);
        actual=std::to_underlying(v.world.status(kind,request));break;
      }
      case 5: {auto const kind{word()};v.actors[0].dead=read()!=0;v.actors[0].disabled=read()!=0;actual=std::to_underlying(v.world.status(kind));break;}
      case 6: v.actors[1].damage_fraction=word();v.danger(read()!=0);v.groups[0].order=6;v.groups[1].waiting=true;actual=std::to_underlying(v.world.status(6,true));break;
      case 7: {
        auto const state{read()};auto const ratio{word()};auto const frame{read()};if(frame==0) clock={};
        v.actors[0].dead=state==0;v.actors[0].disabled=state==1;v.actors[0].arrived=state==2;if(state==3)v.actors[0].position.x=63000;
        auto const result{clock.advance(ratio,[&]{return v.world.status(0);})};
        check("clock result",read(),std::to_underlying(result.value_or(mission_status::none)));
        check("poll",read(),clock.poll_remaining);check("confirm",read(),clock.confirm_remaining);check("pending",read()!=0,clock.pending);check("reported",read(),std::to_underlying(clock.reported));continue;
      }
      default:f.fail("fixture case");
    }
    check("outcome",read(),actual);
  }
  auto const grades{read()};
  for(int i{0};i!=grades;++i,++trial) {
    fixture v;for(auto &group:v.groups)group.side=0;auto const kind{word()};auto const own{read()},target{read()},flags{read()};
    v.groups[0].route_complete=read()!=0;v.groups[1].waiting=read()==0;
    static constexpr std::array<int16_t,5> raw_own{0,51,129,193,255},raw_target{0,50,128,192,255};
    v.actors[0].damage_fraction=raw_own[static_cast<size_t>(own)];v.actors[0].dead=own==4;
    v.actors[1].damage_fraction=raw_target[static_cast<size_t>(target)];v.actors[1].dead=target==4;
    v.actors[0].alerted=(flags&1)!=0;v.actors[0].arrived=(flags&2)!=0;v.actors[0].rescued=(flags&4)!=0;v.actors[0].route_checked=(flags&8)!=0;
    check("debrief grade",read(),v.world.result_grade(0,kind));
  }
  auto const updates{read()};
  for(int i{0};i!=updates;++i,++trial) {
    player_objective_input input;input.leader=read()!=0;input.kind=word();input.selected_objective=read()==1;
    mission_route route;auto const n{read()};for(int j{0};j!=n;++j)route.points.push_back(word());auto cursor{word()};
    std::array<vec3<int32_t>,5> points;for(auto &p:points)p=point();input.position=point();input.anchor=point();input.rescue_position=vec3<int32_t>{input.anchor};
    input.heading=static_cast<uint16_t>(read());input.torso=word();input.target_dead=read()!=0;input.target_waiting=read()!=0;
    player_objective_progress state{read()!=0,read()!=0,read()!=0,read(),read()};auto const ratio{word()};std::vector<int16_t> events;
    advance_player_objective(state,input,&route,points,cursor,ratio,[&](int16_t code){events.push_back(code);});
    check("events",read(),static_cast<int>(events.size()));for(auto const code:events)check("event",read(),code);
    check("cursor",read(),cursor);check("arrived",read()!=0,state.arrived);check("rescued",read()!=0,state.rescued);check("announced",read()!=0,state.target_announced);check("stage",read(),state.rescue_stage);check("rescue timer",read(),state.rescue_remaining);
  }
  auto const worlds{read()};
  for(int i{0};i!=worlds;++i,++trial) {
    std::array<objective_actor,8> actors;
    std::array<std::array<uint8_t,8>,8> known;
    std::array<objective_group,4> groups;
    std::array<std::array<size_t,2>,4> members;
    for(size_t a{0};a!=actors.size();++a) {
      auto &actor{actors[a]};actor.group=static_cast<size_t>(read());actor.position=point();actor.damage_fraction=word();
      actor.dead=read()!=0;actor.disabled=read()!=0;actor.removed=read()!=0;actor.unavailable=read()!=0;actor.seen=read()!=0;
      actor.arrived=read()!=0;actor.rescued=read()!=0;actor.alerted=read()!=0;actor.route_checked=read()!=0;
      auto const selected{read()};if(selected>=0)actor.selected=static_cast<size_t>(selected);
      for(auto &value:known[a]) value=static_cast<uint8_t>(read());
      actor.known=known[a];
    }
    for(size_t g{0};g!=groups.size();++g) {
      auto &group{groups[g]};group.order=word();group.side=static_cast<uint8_t>(read());group.waiting=read()!=0;group.route_complete=read()!=0;
      bool const target_group{read()!=0};auto const target{static_cast<size_t>(read())};
      if(target_group)group.target_group=target;else group.target_actor=target;
      members[g]={g*2,g*2+1};group.members=members[g];
    }
    objective_world const world{actors,groups,0,{{-1000,-2000},{3000,4000}}};auto const kind{word()};bool const request{read()!=0};
    for(size_t a{0};a!=actors.size();++a)check("world safe",read()!=0,world.safe(a));
    for(size_t g{0};g!=groups.size();++g)check("world grade",read(),world.group_grade(g));
    for(size_t g{0};g!=groups.size();++g)check("world target safety",read()!=0,world.target_safe(g));
    for(size_t g{0};g!=groups.size();++g)check("world target grade",read(),world.target_grade(g));
    for(size_t g{0};g!=groups.size();++g)check("world target route",read()!=0,world.target_route_complete(g));
    check("world status",read(),std::to_underlying(world.status(kind,request)));
    for(size_t g{0};g!=groups.size();++g)check("world result grade",read(),world.result_grade(g,kind));
  }
  f.require_end();std::cout<<count<<" original outcomes/clocks, "<<grades<<" debrief grades, "<<updates<<" player objective updates, "<<worlds<<" objective worlds match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
