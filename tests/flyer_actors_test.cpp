#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/flyer_actor.h"
#include "simulation/model_math.h"

namespace {
using namespace earthsiege;
struct state {
  std::array<std::vector<int16_t>,4> damage;
  bool dead, removed, detectable;
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  vec3<int16_t> velocity, rates;
  random_state::state random;
};
state read_state(binary_reader &file) {
  state s;
  for(auto &list : s.damage) { list.resize(file.integer<uint16_t>()); for(auto &v : list) v = file.integer<int16_t>(); }
  s.dead = file.integer<uint8_t>() != 0; s.removed = file.integer<uint8_t>() != 0; s.detectable = file.integer<uint8_t>() != 0;
  s.position = vec3<int32_t>{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
  s.angles = vec3<uint16_t>{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
  s.velocity = vec3<int16_t>{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
  s.rates = vec3<int16_t>{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
  for(auto &v : s.random.words) v = file.integer<uint16_t>();
  s.random.first = file.integer<uint8_t>(); s.random.second = file.integer<uint8_t>(); return s;
}
void compare(flyer_actor const &actor, random_state const &rng, state const &s, std::string const &label) {
  auto const &damage{actor.damage()};
  if(damage.internal != s.damage[0] || damage.armour != s.damage[1] || damage.present != s.damage[2] || damage.cells != s.damage[3]) throw std::runtime_error{label + " damage"};
  if(!std::ranges::equal(actor.instance().cell_frames(),s.damage[3])) throw std::runtime_error{label + " cells"};
  if(actor.dead() != s.dead || actor.dead() != s.removed || actor.is_detectable() != s.detectable) throw std::runtime_error{label + " flags"};
  if(actor.placement().translation != s.position || actor.angles() != s.angles || actor.local_velocity() != s.velocity || actor.angular_velocity() != s.rates) throw std::runtime_error{label + " movement state"};
  auto const &a{rng.snapshot()}; if(a.words != s.random.words || a.first != s.random.first || a.second != s.random.second) throw std::runtime_error{label + " RNG"};
}
using event = std::array<int32_t,5>;
std::vector<event> read_events(binary_reader &file) {
  std::vector<event> result(file.integer<uint16_t>()); for(auto &row : result) for(auto &v : row) v = file.integer<int32_t>(); return result;
}
}
int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const definition{read_flyer_definition(resources,"SKIMMER")};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESFLYER1") file.fail("fixture version");
  auto const initial{read_state(file)};
  auto make_actor{[&]{ return std::make_unique<flyer_actor>(definition,initial.position,initial.angles); }};
  auto actor{make_actor()}; random_state rng{initial.random}; compare(*actor,rng,initial,"initial");
  auto const tape{read_input_tape(read_file(resources.root() / "TAPES/DEMO3.TAP"))};
  auto header{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]).header}; header.zone = -1;
  scenery_world world{resources,header};
  auto const shading{read_distance_shade_bindings(resources.read("simvol0","dat/SHADES.DAT"))};
  framebuffer frame; scenery_renderer renderer{frame,world,shading}; std::array const lights{default_world_light()};
  auto const frames{file.integer<uint16_t>()}; unsigned int mismatches{0};
  for(unsigned int i{0}; i != frames; ++i) {
    vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()}; auto const crc{file.integer<uint32_t>()};
    std::array const models{actor->model()}; frame.clear(); renderer.draw(position,angles,lights,false,models);
    if(crc32(frame.pixels()) != crc) { if(mismatches++ == 0) frame.save("flyer-mismatch.ppm",world.colours()); std::cerr << "Frame " << i << " differs\n"; }
  }
  std::vector<event> actual;
  flyer_damage_events events;
  events.explosion = [&](int16_t id, vec3<int32_t> const &p, random_state &){ actual.push_back({0,id,p.x,p.y,p.z}); };
  events.debris = [&](int16_t id, vec3<int32_t> const &p, random_state &){ actual.push_back({1,id,p.x,p.y,p.z}); };
  events.kill_credit = [&]{ actual.push_back({2,0,0,0,0}); };
  events.destroyed = [&]{ actual.push_back({3,0,0,0,0}); };
  events.locked_target_destroyed = [&]{ actual.push_back({4,3,0,0,0}); };
  auto const damages{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != damages; ++i) {
    auto const amount{file.integer<int16_t>()}; bool const result{file.integer<uint8_t>() != 0}; auto const expected{read_state(file)}; auto const expected_events{read_events(file)};
    actual.clear(); if(actor->direct_damage(amount,rng,events) != result) file.fail("direct damage result");
    compare(*actor,rng,expected,"damage " + std::to_string(i)); if(actual != expected_events) file.fail("direct damage events");
  }
  auto const rays{file.integer<uint16_t>()}; unsigned int hits{0};
  for(unsigned int i{0}; i != rays; ++i) {
    auto const stage{file.integer<uint8_t>()}; vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()}; auto const heading{file.integer<uint16_t>()};
    impact_request request; request.transform = rotation_transform({0,0,heading}); request.transform.translation = position;
    request.reach = file.integer<int32_t>(); request.radius = file.integer<int16_t>(); request.damage.armour = file.integer<int16_t>(); request.damage.armour_effects = {8,9,10,11};
    impact_conditions conditions; conditions.attacker_side = file.integer<uint8_t>(); conditions.difficulty = file.integer<int16_t>(); conditions.difficulty_override = file.integer<uint8_t>() != 0;
    bool const locked{file.integer<uint8_t>() != 0}; auto const result{file.integer<int32_t>()}; auto const expected{read_state(file)}; auto const expected_events{read_events(file)};
    actor = make_actor(); rng = random_state{initial.random};
    if(stage) actor->direct_damage(stage == 1 ? 400 : 500,rng,events);
    actual.clear(); auto const hit{actor->receive_impact(request,conditions,rng,events,locked)};
    auto const label{"ray " + std::to_string(i)};
    if(hit.value_or(0) != result) file.fail(label + " hit distance");
    compare(*actor,rng,expected,label); if(actual != expected_events) file.fail(label + " events"); hits += hit.has_value();
  }
  file.require_end();
  if(mismatches) file.fail(std::to_string(mismatches) + " skimmer frames differ");
  if(frames != 32 || rays != 768 || hits != 107 || damages != 10) file.fail("flyer coverage differs");
  std::cout << frames << " skimmer VGA frames, " << rays << " rays and " << damages << " direct damage steps agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
