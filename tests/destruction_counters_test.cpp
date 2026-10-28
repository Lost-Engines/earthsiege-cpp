#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/mission_activity.h"

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2)return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESCOUNT1")file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};size_t total{0};
  for(size_t trial{0};trial!=count;++trial) {
    auto const members{file.integer<uint16_t>()},selected{file.integer<uint16_t>()};
    mission_script script; script.hercs.resize(members); script.groups.resize(1);
    auto &actor{script.hercs[selected]};actor.counter=file.integer<int16_t>();actor.counter_operation=file.integer<int16_t>();
    script.groups[0].extra[1]=file.integer<int16_t>();script.groups[0].extra[2]=file.integer<int16_t>();script.groups[0].target_type=target_kind::none;
    mission_placement placement;placement.groups.resize(1);
    std::vector<observed_actor> states;
    for(size_t i{0};i!=members;++i) {
      placement.actors[0].push_back({.source{i},.position{},.angles{},.group{0},.ordinal{i},.weapons{}});
      placement.groups[0].members.push_back({actor_kind::herc,i});
      states.push_back({.position{},.dead{file.integer<uint8_t>()!=0},.disabled{file.integer<uint8_t>()!=0}});
    }
    mission_counters initial; for(auto &x:initial)x=file.integer<int16_t>();mission_activity activity{script,placement,initial};
    auto const repeats{file.integer<uint16_t>()};
    for(size_t r{0};r!=repeats;++r) {
      activity.notify_destroyed(selected,states);
      for(auto const actual:activity.variables())if(actual!=file.integer<int16_t>())file.fail("notification "+std::to_string(trial)+"/"+std::to_string(r));
      ++total;
    }
  }
  file.require_end();if(total!=2160)file.fail("counter coverage");
  std::cout<<total<<" original destruction notifications agree\n";return EXIT_SUCCESS;
} catch(std::exception const &e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
