#include "mission_compiler.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>
#include "resources/mission_records.h"
#include "simulation/random_state.h"

namespace earthsiege {
namespace {

struct source_selector {
  int16_t id{};
  int16_t condition{};
  int16_t variant{};
  int16_t reserved{};
};

template<typename T>
struct source_record {
  source_selector selector;
  T value;
};

struct source_point {
  int16_t sum{};
  vec3<world_coordinate> position;
};

template<typename T>
auto reference(std::vector<source_record<T>> const &records, int16_t id, bool include_templates = false)->int16_t {
  /// Original lookups optionally count unnamed templates when forming an index.
  if(id == -1) return -1;
  int16_t index{};
  for(auto const &record : records) {
    if(record.selector.id == -1) {
      if(include_templates) ++index;
    } else {
      if(record.selector.id == id) return index;
      ++index;
    }
  }
  return -1;
}

template<typename T>
void update(T &target, T value, T sentinel) {
  if(value != sentinel) target = value;
}

template<typename T, size_t Count>
void update(std::array<T, Count> &target, std::array<T, Count> const &source, T sentinel) {
  for(unsigned int index{}; index < Count; ++index) update(target[index], source[index], sentinel);
}

void merge(mission_trigger &target, mission_trigger const &source) {
  update(target.mode, source.mode, int16_t{-1});
  update(target.test, source.test, int16_t{-1});
  update(target.areas, source.areas, int16_t{-1});
  update(target.counter, source.counter, int16_t{-1});
  update(target.counter_operation, source.counter_operation, int16_t{-1});
}

void merge(herc_spawn &target, herc_spawn const &source) {
  update(target.field_mask, source.field_mask, int16_t{});
  update(target.weapons, source.weapons, int16_t{-1});
  update(target.ammunition, source.ammunition, int16_t{5});
  update(target.chassis, source.chassis, int16_t{-1});
  update(target.position, source.position, int16_t{-1});
  update(target.heading, source.heading, int16_t{-1});
  update(target.counter, source.counter, int16_t{-1});
  update(target.counter_operation, source.counter_operation, int16_t{-1});
  update(target.skill, source.skill, int16_t{2});
  update(target.trigger_a, source.trigger_a, int16_t{-1});
  update(target.trigger_b, source.trigger_b, int16_t{-1});
  update(target.initial_condition, source.initial_condition, int16_t{100});
}

void merge(flyer_spawn &target, flyer_spawn const &source) {
  update(target.field_mask, source.field_mask, int16_t{});
  update(target.position, source.position, int16_t{-1});
  update(target.heading, source.heading, int16_t{-1});
  update(target.type, source.type, int16_t{-1});
  update(target.counter, source.counter, int16_t{-1});
  update(target.counter_operation, source.counter_operation, int16_t{-1});
  update(target.trigger_a, source.trigger_a, int16_t{-1});
  update(target.trigger_b, source.trigger_b, int16_t{-1});
  update(target.initial_value, source.initial_value, int16_t{100});
}

void merge(base_spawn &target, base_spawn const &source) {
  update(target.type, source.type, int16_t{-1});
  update(target.position, source.position, int16_t{-1});
  update(target.heading, source.heading, int16_t{-1});
  update(target.counter, source.counter, int16_t{-1});
  update(target.counter_operation, source.counter_operation, int16_t{-1});
  update(target.trigger_a, source.trigger_a, int16_t{-1});
  update(target.trigger_b, source.trigger_b, int16_t{-1});
  update(target.initial_value, source.initial_value, int16_t{100});
}

void merge(group_spawn &target, group_spawn const &source) {
  update(target.field_mask, source.field_mask, int16_t{});
  update(target.members, source.members, int16_t{-1});
  update(target.kind, source.kind, static_cast<actor_kind>(-1));
  update(target.formation, source.formation, int16_t{-1});
  update(target.argument, source.argument, int16_t{-1});
  update(target.position, source.position, int16_t{-1});
  update(target.heading, source.heading, int16_t{-1});
  update(target.route, source.route, int16_t{-1});
  update(target.mode, source.mode, int16_t{-1});
  update(target.target_type, source.target_type, target_kind::none);
  update(target.target, source.target, int16_t{-1});
  update(target.trigger, source.trigger, int16_t{-1});
  update(target.extra, source.extra, int16_t{-1});
}

template<typename T>
auto exported(std::vector<source_record<T>> const &records)->std::vector<T> {
  std::vector<T> result;
  for(auto const &record : records) {
    if(record.selector.id != -1) result.push_back(T{record.value});
  }
  return result;
}

class mission_compiler {
  binary_reader file;
  random_state &random;
  std::vector<mission_condition> active;
  mission_header header;
  std::vector<source_record<source_point>> points;
  std::vector<source_record<int16_t>> headings;
  std::vector<source_record<mission_route>> routes;
  std::vector<source_record<trigger_area>> areas;
  std::vector<source_record<mission_trigger>> triggers;
  std::vector<source_record<herc_spawn>> hercs;
  std::vector<source_record<flyer_spawn>> flyers;
  std::vector<source_record<base_spawn>> bases;
  std::vector<source_record<group_spawn>> groups;

  auto choose_variant(int16_t group)->int16_t {
    /// GO 0x318d5 binds each group at the point its source record requests it.
    for(auto &condition : active) {
      auto const *rule{std::get_if<random_group>(&condition.rule)};
      if(!rule || rule->group != group) continue;
      condition.scratch = random.bounded(rule->limit);
      for(auto const &candidate : active) {
        auto const *interval{std::get_if<random_interval>(&candidate.rule)};
        if(interval && candidate.parent == condition.id && interval->first <= condition.scratch && condition.scratch <= interval->last) return candidate.id;
      }
    }
    throw std::runtime_error{"mission random group has no eligible variant"};
  }

  template<typename T>
  auto variant(std::vector<source_record<T>> const &records, int16_t group)->T const & {
    int16_t const condition{choose_variant(group)};
    for(auto const &record : records) {
      if(record.selector.condition == condition) return record.value;
    }
    throw std::runtime_error{"mission variant has no preceding template"};
  }

  template<typename T, typename Read, typename Prepare, typename Merge>
  void load(std::vector<source_record<T>> &records, bool reserved, Read read, Prepare prepare, Merge combine, bool physical_index = false) {
    auto const count{file.integer<uint16_t>()};
    if(count > std::numeric_limits<int16_t>::max()) file.fail("source table exceeds signed reference range");
    records.reserve(count);
    for(unsigned int index{}; index < count; ++index) {
      source_selector selector{
        .id{file.integer<int16_t>()},
        .condition{file.integer<int16_t>()},
        .variant{file.integer<int16_t>()},
        .reserved{reserved ? file.integer<int16_t>() : int16_t{}},
      };
      source_record<T> entry{
        .selector{selector},
        .value{read()},
      };
      if(!condition_enabled(active, selector.condition)) continue;
      prepare(entry);
      auto const duplicate{reference(records, selector.id, physical_index)};
      if(duplicate == -1) records.push_back(std::move(entry));
      else combine(records.at(static_cast<unsigned int>(duplicate)), entry);
    }
  }

  template<typename T>
  static void replace(source_record<T> &target, source_record<T> const &source) { target = source; }

  template<typename T>
  static void overlay(source_record<T> &target, source_record<T> const &source) { merge(target.value, source.value); }

  void settings() {
    auto const count{file.integer<uint16_t>()};
    for(unsigned int index{}; index < count; ++index) {
      auto const condition{file.integer<int16_t>()};
      std::array<int16_t, 10> patch{};
      for(auto &value : patch) value = file.integer<int16_t>();
      if(!condition_enabled(active, condition)) continue;
      update(header.terrain, static_cast<terrain_kind>(patch[0]), terrain_kind::temperate);
      update(header.zone, patch[1], int16_t{});
      // patch[2] changes a shell scratch value; campaign mode is supplied separately.
      update(header.objective_kind, patch[3], int16_t{});
      update(header.objective_variant, patch[4], int16_t{});
      update(header.unlimited_weapons, patch[5], int16_t{});
      update(header.invulnerable_player, patch[6], int16_t{});
      update(header.difficulty, patch[7], int16_t{});
      update(header.world_variant, patch[8], int16_t{});
      update(header.palette_variant, patch[9], int16_t{});
    }
  }

  void coordinates() {
    load(points, true, [this]{
      return source_point{
        .sum{file.integer<int16_t>()},
        .position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()},
      };
    }, [this](auto &entry){
      if(entry.selector.variant != -1) entry.value.position = variant(points, entry.selector.variant).position;
      if(entry.value.sum) {
        auto const first{reference(points, static_cast<int16_t>(entry.value.position.x))};
        auto const second{reference(points, static_cast<int16_t>(entry.value.position.y))};
        if(first < 0 || second < 0) file.fail("point sum refers to an unavailable point");
        auto const &a{points.at(static_cast<unsigned int>(first)).value.position};
        auto const &b{points.at(static_cast<unsigned int>(second)).value.position};
        auto const add{[](int32_t left, int32_t right){ return std::bit_cast<int32_t>(std::bit_cast<uint32_t>(left) + std::bit_cast<uint32_t>(right)); }};
        entry.value.position = vec3<world_coordinate>{add(a.x, b.x), add(a.y, b.y), add(a.z, b.z)};
      }
    }, replace<source_point>);
    load(headings, true, [this]{ return file.integer<int16_t>(); }, [this](auto &entry){
      if(entry.selector.variant != -1) entry.value = variant(headings, entry.selector.variant);
    }, replace<int16_t>);
    load(routes, true, [this]{ return read_routes_record(file); }, [this](auto &entry){
      for(auto &point : entry.value.points) point = reference(points, point);
      if(entry.selector.variant != -1) entry.value = variant(routes, entry.selector.variant);
    }, replace<mission_route>);
    load(areas, false, [this]{ return read_areas_record(file); }, [this](auto &entry){
      entry.value.first_point = reference(points, entry.value.first_point);
      if(entry.value.kind == 0) entry.value.second_point_or_radius = reference(points, entry.value.second_point_or_radius);
    }, replace<trigger_area>);
    load(triggers, false, [this]{ return read_triggers_record(file); }, [this](auto &entry){
      for(auto &area : entry.value.areas) area = reference(areas, area);
    }, overlay<mission_trigger>);
  }

  template<typename T>
  void actor(source_record<T> &entry, std::vector<source_record<T>> const &records) {
    if(entry.selector.variant != -1) entry.value = variant(records, entry.selector.variant);
    else {
      entry.value.position = reference(points, entry.value.position);
      entry.value.heading = reference(headings, entry.value.heading);
      entry.value.trigger_a = reference(triggers, entry.value.trigger_a);
      entry.value.trigger_b = reference(triggers, entry.value.trigger_b);
    }
  }

  void actors() {
    load(hercs, true, [this]{ return read_hercs_record(file); }, [this](auto &entry){ actor(entry, hercs); }, overlay<herc_spawn>, true);
    load(flyers, true, [this]{ return read_flyers_record(file); }, [this](auto &entry){ actor(entry, flyers); }, overlay<flyer_spawn>, true);
    load(bases, true, [this]{ return read_bases_record(file); }, [this](auto &entry){ actor(entry, bases); }, overlay<base_spawn>, true);
    load(groups, false, [this]{ return read_groups_record(file); }, [this](auto &entry){
      auto &value{entry.value};
      value.position = reference(points, value.position);
      value.heading = reference(headings, value.heading);
      value.route = reference(routes, value.route);
      value.trigger = reference(triggers, value.trigger);
      switch(value.target_type) {
        case target_kind::none: break;
        case target_kind::group: value.target = reference(groups, value.target); break;
        case target_kind::herc: value.target = reference(hercs, value.target); break;
        case target_kind::flyer: value.target = reference(flyers, value.target); break;
        case target_kind::base: value.target = reference(bases, value.target); break;
      }
      for(auto &member : value.members) {
        switch(value.kind) {
          case actor_kind::herc: member = reference(hercs, member); break;
          case actor_kind::flyer: member = reference(flyers, member); break;
          case actor_kind::base: member = reference(bases, member); break;
        }
      }
    }, overlay<group_spawn>);
  }

public:
  mission_compiler(bytes source, std::span<int16_t const> flags, random_state &generator, int16_t campaign_mode) :
    file{source, "source mission"},
    random{generator} {
    if(file.integer<uint16_t>() != 5) file.fail("unsupported source mission revision");
    auto const conditions{read_conditions(file)};
    active = evaluate_conditions(conditions, flags, random);
    header.terrain = terrain_kind::temperate;
    header.campaign_mode = campaign_mode;
  }

  auto compile()->compiled_mission {
    settings();
    coordinates();
    actors();
    file.require_end();
    mission_script script;
    script.header = header;
    for(auto const &point : points) {
      if(point.selector.id != -1) script.points.push_back(vec3<world_coordinate>{point.value.position});
    }
    script.headings = exported(headings);
    script.routes = exported(routes);
    script.areas = exported(areas);
    script.triggers = exported(triggers);
    script.hercs = exported(hercs);
    script.flyers = exported(flyers);
    script.bases = exported(bases);
    script.groups = exported(groups);
    return compiled_mission{
      .script{std::move(script)},
      .conditions{std::move(active)},
    };
  }
};

} // namespace

auto compile_mission(bytes source, std::span<int16_t const> campaign_flags, random_state &random, int16_t campaign_mode)->compiled_mission {
  return mission_compiler{source, campaign_flags, random, campaign_mode}.compile();
}

} // namespace earthsiege
