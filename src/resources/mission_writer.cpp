#include "resources/mission_script.h"
#include "resources/player_roster.h"
#include "resources/binary_writer.h"
#include <stdexcept>
#include <utility>
namespace earthsiege {
namespace {
template<typename R> void words(binary_writer &file,R const &values){for(auto v:values)file.integer(v);}
template<typename R,typename F> void table(binary_writer &file,R const &values,F write){
  if(values.size()>65535)throw std::runtime_error{"mission table exceeds original word count"};
  file.integer<uint16_t>(static_cast<uint16_t>(values.size()));for(auto const &v:values)write(v);
}
}
std::vector<uint8_t> write_mission_script(mission_script const &s) {
  binary_writer f;auto const &h{s.header};f.integer(std::to_underlying(h.terrain));f.integer(h.zone);f.integer(h.campaign_mode);
  f.integer(h.objective_kind);f.integer(h.objective_variant);f.integer(h.unlimited_weapons);f.integer(h.invulnerable_player);f.integer(h.difficulty);f.integer(h.world_variant);f.integer(h.palette_variant);
  table(f,s.points,[&](auto const &p){f.integer(p.x);f.integer(p.y);f.integer(p.z);});
  table(f,s.headings,[&](auto v){f.integer(v);});
  table(f,s.routes,[&](auto const &r){table(f,r.points,[&](auto v){f.integer(v);});});
  table(f,s.areas,[&](auto const &a){f.integer(a.kind);f.integer(a.first_point);f.integer(a.second_point_or_radius);});
  table(f,s.triggers,[&](auto const &t){f.integer(t.mode);f.integer(t.test);words(f,t.areas);f.integer(t.counter);f.integer(t.counter_operation);});
  table(f,s.hercs,[&](auto const &a){words(f,a.field_mask);f.integer(a.chassis);words(f,a.weapons);f.integer(a.position);f.integer(a.heading);f.integer(a.counter);f.integer(a.counter_operation);words(f,a.ammunition);f.integer(a.skill);f.integer(a.trigger_a);f.integer(a.trigger_b);f.integer(a.initial_condition);});
  table(f,s.flyers,[&](auto const &a){words(f,a.field_mask);f.integer(a.position);f.integer(a.heading);f.integer(a.type);f.integer(a.counter);f.integer(a.counter_operation);f.integer(a.trigger_a);f.integer(a.trigger_b);f.integer(a.initial_value);});
  table(f,s.bases,[&](auto const &a){f.integer(a.type);f.integer(a.position);f.integer(a.heading);f.integer(a.counter);f.integer(a.counter_operation);f.integer(a.trigger_a);f.integer(a.trigger_b);f.integer(a.initial_value);});
  table(f,s.groups,[&](auto const &g){words(f,g.field_mask);f.integer(std::to_underlying(g.kind));f.integer(g.formation);f.integer(g.argument);f.integer(g.position);f.integer(g.heading);f.integer(g.route);words(f,g.members);f.integer(g.mode);f.integer(std::to_underlying(g.target_type));f.integer(g.target);f.integer(g.trigger);words(f,g.extra);});
  return std::move(f).finish();
}
std::vector<uint8_t> write_player_roster(player_roster const &roster) {
  if(roster.hercs.size()>50)throw std::runtime_error{"player roster exceeds original pool"};
  binary_writer f;f.integer(roster.selected);f.integer<int16_t>(static_cast<int16_t>(roster.hercs.size()));
  for(auto const &h:roster.hercs){if(h.weapons.size()>12||h.weapons.size()!=h.weapon_values.size())throw std::runtime_error{"invalid roster weapons"};
    f.integer(h.slot);f.integer(h.field);f.integer(h.chassis);f.integer<int16_t>(static_cast<int16_t>(h.weapons.size()));words(f,h.weapons);words(f,h.weapon_values);f.integer(h.extra);
    words(f,h.condition.armour);words(f,h.condition.internal);words(f,h.condition.equipment);
  }
  f.append(roster.flags);return std::move(f).finish();
}
std::vector<uint8_t> write_mission_counters(mission_counters const &counters){binary_writer f;words(f,counters);return std::move(f).finish();}
} // namespace earthsiege
