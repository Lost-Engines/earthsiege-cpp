#include "resources/binary_reader.h"
#include "simulation/wingmate_orders.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESWORD01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const read{[&]{return file.integer<int32_t>();}};
  auto const ref{[&]()->std::optional<size_t>{auto const i{read()}; return i<0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(i)};}};
  for(uint32_t trial{};trial<count;++trial) {
    wingmate_order order; order.command=static_cast<uint16_t>(read()); order.issuer=0;
    auto const previous{static_cast<uint16_t>(read())}; auto const chosen{ref()},target{ref()}; order.target=ref();
    auto const group_order{static_cast<int16_t>(read())}; order.position={read(),read(),0};
    std::array<wingmate_order_state,4> actors;
    for(auto &a:actors) {
      a.position={read(),read(),read()}; a.type=static_cast<behavior>(read()); a.unavailable=read()!=0; a.disabled=read()!=0; a.dead=read()!=0;
      a.radar=read()!=0; a.radar_requested=read()!=0; a.ignore_target=read()!=0; a.command=static_cast<int16_t>(read());
      a.designated=ref(); a.selected=ref(); a.stop_status=static_cast<int16_t>(read()); a.command_position={read(),read()}; a.incoming_damage=read(); a.reconsider_remaining=read();
    }
    auto &a{actors[1]}; std::vector<std::pair<int,int>> events; uint16_t stagger{};
    auto const accepted{receive_wingmate_order(actors,1,order,previous,0,0,group_order,target,{
      .select{[&](bool prefer){events.emplace_back(0,prefer);return chosen;}},
      .attack{[&](size_t selected){events.emplace_back(1,static_cast<int>(selected));}},
      .resume{[&]{events.emplace_back(2,0);}},
      .enter{[&](behavior b){events.emplace_back(3,std::to_underlying(b));controller_state state;enter_behavior(state,b,stagger);a.type=b;a.reconsider_remaining=state.reconsider_remaining;}},
      .acknowledge{[&](uint16_t reply){events.emplace_back(4,reply);}},
    })};
    auto const check{[&](char const *label,int32_t expected,int32_t actual){if(expected!=actual){std::cerr<<trial<<' '<<label<<": expected "<<expected<<", actual "<<actual<<'\n';file.fail(label);}}};
    check("accepted",read(),accepted);check("events",read(),static_cast<int>(events.size()));
    for(auto const &[event,value]:events){check("event",read(),event);check("value",read(),value);}
    check("radar",read(),a.radar);check("radar request",read(),a.radar_requested);check("ignore",read(),a.ignore_target);check("command",read(),a.command);
    check("designated",read(),a.designated?static_cast<int>(*a.designated):-1);check("status",read(),a.stop_status);
    check("point x",read(),a.command_position.x);check("point y",read(),a.command_position.y);check("damage",read(),a.incoming_damage);check("clock",read(),a.reconsider_remaining);check("behavior",read(),std::to_underlying(a.type));
  }
  file.require_end();std::cout<<count<<" original wingmate order branches match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
