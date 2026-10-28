#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)}; if(std::string(magic.begin(),magic.end()) != "ESTARG01") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()}; size_t cases{0},centres{0}; ground_catalogue ground{resources};
  for(size_t w{0}; w != worlds; ++w) {
    auto const demo{file.integer<uint16_t>()}; random_state::state rng; for(auto &x:rng.words)x=file.integer<uint16_t>(); rng.first=file.integer<uint8_t>(); rng.second=file.integer<uint8_t>();
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    mission_session session{resources,script,roster,random_state{rng}}; auto const hercs{session.hercs()}; auto const flyers{session.flying_actors()}; auto const bases{session.ground_actors()};
    auto const actors{file.integer<uint16_t>()}; if(actors != hercs.size()+flyers.size()+bases.size())file.fail("actor count");
    for(size_t a{0};a != actors;++a) {
      auto const kind{file.integer<int16_t>()},trials{file.integer<int16_t>()};
      for(int trial{0};trial != trials;++trial) {
        vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
        vec3<int32_t> const origin{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        vec3<uint16_t> const aim{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
        auto const selected{file.integer<int16_t>()},n{file.integer<int16_t>()};
        auto const label{std::to_string(demo)+"/"+std::to_string(a)+"/"+std::to_string(trial)};
        herc_actor *herc{nullptr};
        // Views are read-only in production. The owning session is mutable here;
        // move its actor to each original-code fixture pose without duplicating construction.
        if(kind==0) { herc=&const_cast<herc_actor &>(hercs.at(a).actor); herc->place(position,angles); if(herc->target_component(origin,aim)!=selected)file.fail(label+" selected component"); }
        else if(selected!=-1 || n!=0)file.fail(label+" ordinary target component");
        for(int i{0};i!=n;++i) {
          auto const part{file.integer<int16_t>()}; vec3<int32_t> const point{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
          if(herc->target_position(part)!=point) {
            auto const actual{herc->target_position(part)};
            file.fail(label+" component centre "+std::to_string(part)+" actual "+std::to_string(actual.x)+","+std::to_string(actual.y)+","+std::to_string(actual.z)+" expected "+std::to_string(point.x)+","+std::to_string(point.y)+","+std::to_string(point.z));
          }
          ++centres;
        }
        vec3<int32_t> const fallback{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        if(herc) { if(herc->target_position()!=fallback)file.fail(label+" fallback"); }
        else if(kind==1) {
          auto const &base{bases.at(a-hercs.size()-flyers.size())};
          ground_actor actor{ground,static_cast<size_t>(base.actor.profile().type),base.side,-1,position,angles};
          if(actor.target_position()!=fallback)file.fail(label+" base offset");
        } else if(fallback!=position)file.fail(label+" flyer origin");
        ++cases;
      }
    }
  }
  file.require_end(); if(cases!=1536 || centres==0)file.fail("coverage");
  std::cout<<cases<<" target choices and "<<centres<<" component positions match the original\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr<<e.what()<<'\n'; return EXIT_FAILURE; }
