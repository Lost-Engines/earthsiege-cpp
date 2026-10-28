#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/engagement.h"

namespace {

std::optional<size_t> optional(earthsiege::binary_reader &file) {
  auto const value{file.integer<int16_t>()};
  return value < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(value)};
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESENG001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(size_t row{0}; row != count; ++row) {
    auto const require{[&](bool const value,char const *field){ if(!value) file.fail("engagement " + std::to_string(row) + ": " + field); }};
    auto const mode{static_cast<earthsiege::engagement_mode>(file.integer<uint8_t>())};
    bool const leader{file.integer<uint8_t>() != 0};
    earthsiege::engagement_state state;
    state.command = file.integer<int16_t>(); auto const order{file.integer<int16_t>()};
    state.unavailable = file.integer<uint8_t>(); state.disabled = file.integer<uint8_t>(); state.dead = file.integer<uint8_t>();
    bool const prior{file.integer<uint8_t>() != 0}; auto const selection{optional(file)};
    bool const designated{file.integer<uint8_t>() != 0}; auto const kind{file.integer<int16_t>()};
    state.controller.target_search_remaining = file.integer<int16_t>(); auto const ratio{file.integer<int16_t>()}; auto stagger{file.integer<uint16_t>()};
    bool const flyer{mode == earthsiege::engagement_mode::flyer_search || mode == earthsiege::engagement_mode::flyer_patrol};
    state.controller.type = flyer ? earthsiege::behavior::flyer_patrolling : earthsiege::behavior::patrolling;
    state.aimed_component = 123;
    std::array<earthsiege::target_relationship,4> relationships;
    if(prior) relationships[0].selected = 2;
    relationships[1].attackers = 3; relationships[2].attackers = 4;
    std::vector<size_t> targets;
    if(designated && kind >= 0 && kind <= 3 && selection) targets.push_back(*selection);
    earthsiege::target_objective objective{.order{order},.actors{targets},.representative{},.leader{leader ? size_t{0} : size_t{3}}};
    std::vector<std::pair<uint8_t,uint8_t>> events;
    earthsiege::patrol_engagement(mode,state,0,relationships,objective,ratio,stagger,{
      .navigate{[&]{ events.emplace_back(0,0); }},
      .select{[&](earthsiege::target_selection_flags flags){ events.emplace_back(1,static_cast<uint8_t>(flags)); return selection; }},
      .attack{[&]{ events.emplace_back(2,0); }},
    });
    require(relationships[0].selected == optional(file),"selected target");
    require(relationships[0].changed == (file.integer<uint8_t>() != 0),"target changed");
    require(relationships[2].attackers == file.integer<int16_t>() && relationships[1].attackers == file.integer<int16_t>(),"assigned counts");
    require(state.controller.target_search_remaining == file.integer<int16_t>(),"search countdown");
    require(state.controller.type == static_cast<earthsiege::behavior>(file.integer<uint8_t>()),"controller");
    require(state.aimed_component == file.integer<int16_t>(),"aimed component");
    require(state.attack_requested == (file.integer<uint8_t>() != 0),"attack request");
    require(state.controller.reconsider_remaining == file.integer<int32_t>(),"reconsider countdown");
    require(stagger == file.integer<uint16_t>(),"world stagger");
    require(state.controller.ticks == file.integer<uint32_t>(),"ticks");
    require(state.controller.first_entry == (file.integer<uint8_t>() != 0) && state.controller.updated == (file.integer<uint8_t>() != 0),"entry flags");
    require(events.size() == file.integer<uint8_t>(),"event count");
    for(auto const &[phase,value] : events) require(phase == file.integer<uint8_t>() && value == file.integer<uint8_t>(),"event order");
  }
  auto const broadcasts{file.integer<uint16_t>()};
  for(size_t row{0}; row != broadcasts; ++row) {
    std::array<earthsiege::engagement_state,6> actors;
    std::array<earthsiege::target_relationship,6> relationships;
    for(auto &actor : actors) actor.controller.type = earthsiege::behavior::flyer_patrolling;
    relationships[0].selected = optional(file);
    uint16_t stagger{0}; unsigned int living{1};
    for(size_t index{3}; index != 6; ++index) {
      actors[index].dead = file.integer<uint8_t>(); relationships[index].selected = optional(file);
      if(!actors[index].dead) ++living;
    }
    relationships[1].attackers = relationships[2].attackers = 20;
    std::array<size_t,4> const members{0,3,4,5};
    earthsiege::begin_flyer_attack(actors,relationships,0,members,stagger);
    if(relationships[2].attackers != file.integer<int16_t>() || relationships[1].attackers != file.integer<int16_t>() || stagger != 13 * living) file.fail("flyer group counts");
    for(auto const member : members) {
      auto const &state{actors[member].controller};
      auto const type{static_cast<earthsiege::behavior>(file.integer<uint8_t>())}; auto const clock{file.integer<int32_t>()}; auto const ticks{file.integer<uint32_t>()};
      bool const updated{file.integer<uint8_t>() != 0}, first{file.integer<uint8_t>() != 0}, changed{file.integer<uint8_t>() != 0};
      if(state.type != type || state.reconsider_remaining != clock || state.ticks != ticks || state.updated != updated || state.first_entry != first || relationships[member].changed != changed) file.fail("flyer group state " + std::to_string(row));
    }
  }
  file.require_end();
  std::cout << count << " original patrol/search decisions and " << broadcasts << " flyer group entries agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n'; return EXIT_FAILURE;
}
