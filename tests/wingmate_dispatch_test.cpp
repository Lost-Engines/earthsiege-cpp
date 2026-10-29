#include "resources/binary_reader.h"
#include "simulation/wingmate_orders.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESWDIS01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};auto const read{[&]{return file.integer<int32_t>();}};
  for(uint32_t trial{};trial<count;++trial) {
    wingmate_order order;order.command=static_cast<uint16_t>(read());auto const fallback{static_cast<int16_t>(read())};
    auto const members{static_cast<size_t>(read())};order.issuer=static_cast<size_t>(read());
    std::array<wingmate_order_state,8> actors;std::array<bool,8> responses;
    for(size_t i{};i<actors.size();++i){auto &a{actors[i]};a.position={read(),read(),read()};a.type=static_cast<behavior>(read());a.dead=read()!=0;a.radar=read()!=0;responses[i]=read()!=0;}
    std::array<size_t,8> group;std::iota(group.begin(),group.end(),0);
    std::vector<std::pair<int,int>> events;
    auto const accepted{dispatch_wingmate_order(actors,std::span{group}.first(members),order,[&](size_t i,uint16_t previous){events.emplace_back(static_cast<int>(i),previous);return responses[i];},fallback)};
    auto const check{[&](char const *label,int expected,int actual){if(expected!=actual){std::cerr<<trial<<' '<<label<<": expected "<<expected<<", actual "<<actual<<'\n';file.fail(label);}}};
    check("accepted",read(),accepted);check("count",read(),static_cast<int>(events.size()));
    for(auto const &[actor,previous]:events){check("actor",read(),actor);check("previous reply",read(),previous);}
  }
  file.require_end();std::cout<<count<<" original group dispatch sequences match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
