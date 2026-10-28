#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "resources/resource_store.h"
#include "simulation/ground_actor.h"
#include "simulation/model_math.h"

namespace {
using namespace earthsiege;
struct state {
  int16_t damage; bool dead, wreck; int16_t bursts, burst_timer, cell_timer;
  std::vector<uint16_t> cells;
  std::vector<model_transform> matrices;
  random_state::state random;
};
state read_state(binary_reader &file) {
  state s{file.integer<int16_t>(), file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), {}, {}, {}};
  s.cells.resize(file.integer<uint16_t>()); for(auto &v : s.cells) v = file.integer<uint16_t>();
  s.matrices.resize(file.integer<uint16_t>());
  for(auto &m : s.matrices) {
    // Packed DOS matrix stores the horizontal 2x2 block before its z column.
    std::array<int16_t, 9> v; for(auto &value : v) value = file.integer<int16_t>();
    m.basis = mat3<int16_t>{v[0],v[1],v[4],v[2],v[3],v[5],v[6],v[7],v[8]};
    m.type = file.integer<uint8_t>(); m.flags = file.integer<uint8_t>();
    m.translation = vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
  }
  for(auto &v : s.random.words) v = file.integer<uint16_t>();
  s.random.first = file.integer<uint8_t>(); s.random.second = file.integer<uint8_t>(); return s;
}
void compare(ground_actor const &actor, random_state const &rng, state const &expected, std::string const &label) {
  if(actor.damage() != expected.damage || actor.dead() != expected.dead || actor.is_wreck() != expected.wreck || actor.remaining_bursts() != expected.bursts || actor.burst_timer() != expected.burst_timer || actor.cell_clock() != expected.cell_timer) throw std::runtime_error{label + " actor state"};
  if(!std::ranges::equal(actor.instance().cell_frames(), expected.cells)) throw std::runtime_error{label + " cells"};
  if(!expected.wreck) {
    auto const actual{actor.instance().transforms()};
    if(actual.size() != expected.matrices.size()) throw std::runtime_error{label + " matrix count"};
    for(size_t i{0}; i != actual.size(); ++i) {
      auto const &a{actual[i]}; auto const &e{expected.matrices[i]};
      if(a.translation != e.translation) throw std::runtime_error{label + " matrix translation"};
      for(unsigned int x{0}; x != 3; ++x) for(unsigned int y{0}; y != 3; ++y) if(a.basis.at(x,y) != e.basis.at(x,y)) throw std::runtime_error{label + " matrix basis " + std::to_string(i)};
    }
  }
  auto const &a{rng.snapshot()}; auto const &e{expected.random};
  if(a.words != e.words || a.first != e.first || a.second != e.second) throw std::runtime_error{label + " random state"};
}
using event = std::array<int32_t, 5>;
std::vector<event> read_events(binary_reader &file) {
  std::vector<event> result(file.integer<uint16_t>()); for(auto &e : result) for(auto &v : e) v = file.integer<int32_t>(); return result;
}
}

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]}; earthsiege::ground_catalogue catalogue{resources};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESGCOMB1") file.fail("fixture version");
  unsigned int operations{0}, rays{0}, hits{0};
  auto const count{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    auto const type{file.integer<int16_t>()}; auto const side{file.integer<uint8_t>()}; auto const initial{read_state(file)};
    auto const make_actor{[&]{ return std::make_unique<earthsiege::ground_actor>(catalogue, static_cast<size_t>(type), side, 100, vec3<int32_t>{0,20000,0}, vec3<uint16_t>{0,0,0}); }};
    std::vector<event> actual;
    earthsiege::ground_damage_events events;
    events.explosion = [&](int16_t id, vec3<int32_t> const &p, earthsiege::random_state &){ actual.push_back({0,id,p.x,p.y,p.z}); };
    events.debris = [&](int16_t id, vec3<int32_t> const &p, earthsiege::random_state &){ actual.push_back({1,id,p.x,p.y,p.z}); };
    events.kill_credit = [&]{ actual.push_back({2,0,0,0,0}); };
    events.destroyed = [&]{ actual.push_back({3,0,0,0,0}); };
    events.attached_smoke = [&](vec3<int16_t> const &p){ actual.push_back({4,p.x,p.y,p.z,0}); };
    auto const prefix{std::to_string(type) + '/' + std::to_string(side)};
    for(unsigned int sequence{0}; sequence != 2; ++sequence) {
      auto actor{make_actor()}; earthsiege::random_state rng{initial.random}; compare(*actor,rng,initial,prefix + " initial");
      auto const steps{file.integer<uint16_t>()};
      for(unsigned int step{0}; step != steps; ++step) {
        auto const op{file.integer<uint8_t>()}; auto const value{file.integer<int16_t>()}; auto const expected{read_state(file)}; auto const expected_events{read_events(file)};
        actual.clear(); if(op) actor->direct_damage(value,rng,events); else actor->advance(value,rng,events);
        auto const label{prefix + '/' + std::to_string(sequence) + '/' + std::to_string(step)};
        compare(*actor,rng,expected,label); if(actual != expected_events) file.fail(label + " events"); ++operations;
      }
    }
    auto const amount{file.integer<uint16_t>()};
    for(unsigned int ray{0}; ray != amount; ++ray) {
      auto const stage{file.integer<uint8_t>()};
      vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()}; auto const heading{file.integer<uint16_t>()};
      earthsiege::impact_request request; request.transform = earthsiege::rotation_transform({0,0,heading}); request.transform.translation = position;
      request.reach = file.integer<int32_t>(); request.radius = file.integer<int16_t>(); request.damage.armour = file.integer<int16_t>(); request.damage.armour_effects = {8,9,10,11};
      earthsiege::impact_conditions conditions; conditions.attacker_side = file.integer<uint8_t>(); conditions.difficulty = file.integer<int16_t>(); conditions.difficulty_override = file.integer<uint8_t>() != 0;
      auto const result{file.integer<int32_t>()}; auto const expected{read_state(file)}; auto const expected_events{read_events(file)};
      auto actor{make_actor()}; earthsiege::random_state rng{initial.random};
      if(stage != 0) {
        actor->direct_damage(static_cast<int16_t>(actor->profile().damage_capacity + 1),rng,events);
        if(stage == 2) for(unsigned int tick{0}; tick != 40; ++tick) actor->advance(256,rng,events);
      }
      actual.clear(); auto const hit{actor->receive_impact(request,conditions,rng,events)};
      auto const label{prefix + "/ray/" + std::to_string(ray)};
      if(hit.value_or(0) != result) file.fail(label + " hit distance " + std::to_string(hit.value_or(0)) + " vs " + std::to_string(result));
      compare(*actor,rng,expected,label); if(actual != expected_events) file.fail(label + " events"); ++rays; hits += hit.has_value();
    }
  }
  file.require_end();
  if(count != 18 || operations != 1332 || rays != 1728 || hits < 100) file.fail("ground combat coverage differs");
  std::cout << operations << " ground lifecycle steps, " << rays << " rays and " << hits << " original hits agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
