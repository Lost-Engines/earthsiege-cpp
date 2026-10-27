#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/hit_dispatch.h"
#include "simulation/model_math.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) { return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; }

earthsiege::random_state seeded() {
  earthsiege::random_state::state state;
  for(size_t i{0}; i != state.words.size(); ++i) state.words[i] = static_cast<uint16_t>((i * 7919 + 12345) & 32767);
  state.first = 31; state.second = 0;
  return earthsiege::random_state{state};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWORLD1") file.fail("fixture version");
  auto const fields{file.integer<uint32_t>()};
  std::map<std::string, earthsiege::model_resource> models;
  unsigned int total{0};
  for(unsigned int field{0}; field != fields; ++field) {
    auto const name{file.cstring()}; auto const root{file.integer<uint16_t>()}; auto const count{file.integer<uint16_t>()};
    auto const split{name.find('/')};
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resources.read(name.substr(0, split), name.substr(split + 1))));
    auto const &model{models.at(name).roots.at(root)};
    auto const &heightfield{*std::get<earthsiege::shape_part>(model.content).collision};
    for(unsigned int row{0}; row != count; ++row) {
      auto const label{name + '/' + std::to_string(root) + " case " + std::to_string(row)};
      earthsiege::impact_request request;
      request.transform = earthsiege::rotation_transform({0, 0, file.integer<uint16_t>()});
      request.transform.translation = point(file); request.reach = file.integer<int32_t>(); request.radius = file.integer<int16_t>();
      request.damage.armour = 100; request.damage.shield = 200; request.damage.ground_effects = {104, 105, 106, 107};
      std::array<earthsiege::scenery_collider, 4> scenery;
      for(auto &object : scenery) {
        auto const position{point(file)};
        object.placement = earthsiege::rotation_transform({0, 0, file.integer<uint16_t>()}); object.placement.translation = position;
        object.field = &heightfield; object.radius = model.header.radius;
      }
      std::array<earthsiege::impact_actor, 6> actors;
      actors[0].side = file.integer<uint8_t>();
      earthsiege::shot_source source;
      source.player = file.integer<uint8_t>() != 0;
      if(file.integer<uint8_t>()) source.target = point(file);
      std::array<size_t, 4> const members{0, 3, 4, 5}; source.group_members = members;
      for(size_t index{3}; index != 6; ++index) {
        actors[index].position = point(file); actors[index].dead = file.integer<uint8_t>() != 0; actors[index].disabled = file.integer<uint8_t>() != 0;
      }
      std::vector<std::vector<int32_t>> events;
      for(size_t index{1}; index != 3; ++index) {
        auto &actor{actors[index]};
        actor.side = file.integer<uint8_t>(); actor.group = file.integer<uint8_t>() ? 0 : index;
        actor.waiting = file.integer<uint8_t>() != 0; actor.dead = file.integer<uint8_t>() != 0;
        actor.disabled = file.integer<uint8_t>() != 0; actor.inactive = file.integer<uint8_t>() != 0; actor.quiet = file.integer<uint8_t>() != 0;
        bool const kill{file.integer<uint8_t>() != 0}; auto const depth{file.integer<int32_t>()}; actor.trigger = index - 1;
        actor.hit = [&, index, kill, depth](earthsiege::impact_request const &ray) -> std::optional<int32_t> {
          bool const hit{depth > 0 && depth < ray.reach};
          events.push_back({0, static_cast<int32_t>(index - 1), ray.reach, hit ? depth : 0});
          if(hit && kill) actors[index].dead = true;
          return hit ? std::optional<int32_t>{depth} : std::nullopt;
        };
      }
      auto const expected_result{file.integer<int32_t>()}; auto const expected_reach{file.integer<int32_t>()};
      bool const expected_enemy_hit{file.integer<uint8_t>() != 0};
      earthsiege::random_state::state expected_random;
      for(auto &word : expected_random.words) word = file.integer<uint16_t>();
      expected_random.first = file.integer<uint8_t>(); expected_random.second = file.integer<uint8_t>();
      auto const event_count{file.integer<uint16_t>()};
      std::vector<std::vector<int32_t>> expected_events;
      for(unsigned int event{0}; event != event_count; ++event) {
        auto const kind{file.integer<uint8_t>()}; std::vector<int32_t> entry{kind};
        unsigned int const arguments{kind == 0 ? 3U : kind == 4 ? 4U : kind == 5 ? 2U : 1U};
        for(unsigned int i{0}; i != arguments; ++i) entry.push_back(file.integer<int32_t>());
        expected_events.push_back(std::move(entry));
      }
      auto random{seeded()};
      earthsiege::impact_world world{actors, scenery, random, {}};
      world.events.trigger = [&](size_t const trigger){ events.push_back({3, static_cast<int32_t>(trigger)}); };
      world.events.react = [&](size_t const victim, size_t const owner, int16_t const damage){
        if(owner != 0 || damage != 100) file.fail(label + " reaction arguments");
        events.push_back({1, static_cast<int32_t>(victim - 1)});
      };
      world.events.friendly_warning = [&](size_t const companion){ events.push_back({2, static_cast<int32_t>(companion - 3)}); };
      world.events.effect = [&](vec3<int32_t> const &position, int16_t const effect){ events.push_back({4, position.x, position.y, position.z, effect}); };
      world.events.obstructed = [&](earthsiege::impact_obstruction const obstruction){
        bool const actor{obstruction.kind == earthsiege::obstruction_kind::actor};
        events.push_back({5, actor ? 0 : 1, static_cast<int32_t>(obstruction.index - (actor ? 1 : 0))});
      };
      auto const result{earthsiege::dispatch_impact(request, source, world)};
      if((result ? *result + 1 : 0) != expected_result || result.value_or(request.reach) != expected_reach) file.fail(label + " retained depth");
      if(events != expected_events) {
        for(auto const &entry : events) { std::cerr << "actual:"; for(auto const v : entry) std::cerr << ' ' << v; std::cerr << '\n'; }
        for(auto const &entry : expected_events) { std::cerr << "expected:"; for(auto const v : entry) std::cerr << ' ' << v; std::cerr << '\n'; }
        file.fail(label + " event order");
      }
      auto const &actual_random{random.snapshot()};
      if(actual_random.words != expected_random.words || actual_random.first != expected_random.first || actual_random.second != expected_random.second) file.fail(label + " effect random state");
      if(actors[0].hit_enemy != expected_enemy_hit) file.fail(label + " owner enemy-hit latch");
      ++total;
    }
  }
  file.require_end();
  if(fields != 140 || total != 3360) file.fail("combined world-impact coverage differs");
  std::cout << total << " original combined world-impact calls agree across " << fields << " scenery fields" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
