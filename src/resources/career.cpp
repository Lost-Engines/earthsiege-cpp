#include "career.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace earthsiege {
namespace {
template<size_t N> void read_words(binary_reader &file,std::array<int16_t,N> &words) {
  for(auto &word:words)word=file.integer<int16_t>();
}
template<size_t N> void write_words(binary_writer &file,std::array<int16_t,N> const &words) {
  for(auto word:words)file.integer(word);
}
void write_condition(binary_writer &file,herc_condition const &c) {
  write_words(file,c.armour);write_words(file,c.internal);write_words(file,c.equipment);
}
void read_condition(binary_reader &file,herc_condition &c) {
  read_words(file,c.armour);read_words(file,c.internal);read_words(file,c.equipment);
}
}
armory read_armory(binary_reader &file) {
  auto const count{file.integer<int16_t>()};
  armory result(count>17?22:17);
  if(count<0||count>22)file.fail("invalid armory type count");
  std::array<bool,22> seen{};
  for(int i{};i<count;++i) {
    auto const type{file.integer<int16_t>()},amount{file.integer<int16_t>()};
    if(type<0||type>=22||seen[type]||amount<0)file.fail("invalid armory stack");
    if(static_cast<size_t>(type)>=result.size())result.resize(22);
    seen[type]=true;auto &values{result[type]};values.resize(static_cast<size_t>(amount));
    for(auto it{values.rbegin()};it!=values.rend();++it)*it=file.integer<int16_t>();
  }
  return result;
}
std::vector<uint8_t> write_armory(armory const &inventory) {
  binary_writer file;file.integer<int16_t>(static_cast<int16_t>(inventory.size()));
  for(int16_t i{};static_cast<size_t>(i)<inventory.size();++i) {
    auto const &values{inventory[i]};
    if(values.size()>32767)throw std::runtime_error{"armory stack exceeds original short count"};
    file.integer(i);file.integer<int16_t>(static_cast<int16_t>(values.size()));
    for(auto value:values)file.integer(value);
  }
  return std::move(file).finish();
}
player_herc *hangar::find(int16_t slot) {
  auto it{std::ranges::find(hercs,slot,&player_herc::slot)};return it==hercs.end()?nullptr:&*it;
}
player_herc const *hangar::find(int16_t slot) const {
  auto it{std::ranges::find(hercs,slot,&player_herc::slot)};return it==hercs.end()?nullptr:&*it;
}
hangar read_hangar(binary_reader &file,game_edition edition) {
  hangar out;
  out.capacity=file.integer<int16_t>();out.selected=file.integer<int16_t>();
  out.construction_slot=file.integer<int16_t>();out.construction_remaining=file.integer<int16_t>();
  out.construction_value=file.integer<int32_t>();auto const count{file.integer<uint16_t>()};
  if(out.capacity<0||out.capacity>50||count>out.capacity)file.fail("invalid hangar capacity");
  for(unsigned i{};i<count;++i) {
    player_herc h;h.slot=file.integer<int16_t>();h.chassis=file.integer<int16_t>();
    auto const n{file.integer<int16_t>()};
    if(h.slot<0||h.slot>=out.capacity||out.find(h.slot)||h.chassis<0||(h.chassis>=8&&!(edition==game_edition::expansion&&h.chassis==16))||n<0||n>10)file.fail("invalid hangar chassis");
    for(int j{};j<n;++j)h.weapons.push_back(file.integer<int16_t>());
    for(int j{};j<n;++j)h.weapon_values.push_back(file.integer<int16_t>());
    h.extra=file.integer<int16_t>();read_condition(file,h.condition);out.hercs.push_back(std::move(h));
  }
  return out;
}
hangar read_hangar(bytes source,game_edition edition) { binary_reader file{source,"hangar"};auto out{read_hangar(file,edition)};file.require_end();return out; }
std::vector<uint8_t> write_hangar(hangar const &roster) {
  if(roster.capacity<0||roster.capacity>50||roster.hercs.size()>static_cast<size_t>(roster.capacity))throw std::runtime_error{"invalid hangar capacity"};
  binary_writer file;file.integer(roster.capacity);file.integer(roster.selected);file.integer(roster.construction_slot);
  file.integer(roster.construction_remaining);file.integer(roster.construction_value);file.integer<uint16_t>(static_cast<uint16_t>(roster.hercs.size()));
  // The original writer scans the slot array, independent of insertion order.
  for(int16_t slot{};slot<roster.capacity;++slot)if(auto h{roster.find(slot)}) {
    if(h->weapons.size()>10||h->weapon_values.size()!=h->weapons.size())throw std::runtime_error{"invalid hangar weapon count"};
    file.integer(slot);file.integer(h->chassis);file.integer<int16_t>(static_cast<int16_t>(h->weapons.size()));
    for(auto w:h->weapons)file.integer(w);
    for(auto w:h->weapon_values)file.integer(w);
    file.integer(h->extra);write_condition(file,h->condition);
  }
  return std::move(file).finish();
}
career_state read_career_state(binary_reader &file,game_edition edition,int16_t battle_set) {
  career_state s;s.valid=file.integer<uint8_t>();s.current=file.integer<uint8_t>();
  s.node_progress=file.integer<int16_t>();s.screen=file.integer<int16_t>();s.node=file.integer<int16_t>();s.step=file.integer<int16_t>();
  s.missions=file.integer<int16_t>();s.mission_kind=file.integer<int16_t>();s.selected_crew=file.integer<int16_t>();s.crew_count=file.integer<int16_t>();s.crew_cursor=file.integer<int16_t>();
  for(auto &c:s.crew) {
    c.name=file.integer<int16_t>();c.variant=file.integer<int16_t>();c.next_name=file.integer<int16_t>();c.rotation=file.integer<int16_t>();c.hangar_slot=file.integer<int16_t>();
    c.active=file.integer<uint8_t>();c.skill=file.integer<int16_t>();c.rank=file.integer<int16_t>();c.health=file.integer<int16_t>();
    read_words(file,c.current);read_words(file,c.total);c.prior_skill=file.integer<int16_t>();c.experience=file.integer<int16_t>();c.budget_share=file.integer<int32_t>();
  }
  s.technology.last_type=file.integer<int16_t>();s.technology.last_index=file.integer<int16_t>();
  auto &technology{s.technology};technology.edition=edition;technology.battle_set=battle_set;
  if(edition==game_edition::expansion){technology.chassis_slots=9;technology.entries.resize(31);if(battle_set){technology.chassis_count=9;technology.weapon_count=22;}}
  auto read=[&](technology_entry &t){t.early=file.integer<int16_t>();t.late=file.integer<int16_t>();t.available=file.integer<uint8_t>();};
  for(size_t i{};i<technology.chassis_count;++i)read(technology.entries[i]);
  for(size_t i{};i<technology.weapon_count;++i)read(technology.entries[technology.chassis_slots+i]);
  s.budget=file.integer<int32_t>();read_words(file,s.counters);s.repair_policy=file.integer<int16_t>();return s;
}
std::vector<uint8_t> write_career_state(career_state const &s) {
  binary_writer file;file.integer(s.valid);file.integer(s.current);file.integer(s.node_progress);file.integer(s.screen);file.integer(s.node);file.integer(s.step);
  file.integer(s.missions);file.integer(s.mission_kind);file.integer(s.selected_crew);file.integer(s.crew_count);file.integer(s.crew_cursor);
  for(auto const &c:s.crew) {
    file.integer(c.name);file.integer(c.variant);file.integer(c.next_name);file.integer(c.rotation);file.integer(c.hangar_slot);file.integer(c.active);
    file.integer(c.skill);file.integer(c.rank);file.integer(c.health);write_words(file,c.current);write_words(file,c.total);file.integer(c.prior_skill);file.integer(c.experience);file.integer(c.budget_share);
  }
  file.integer(s.technology.last_type);file.integer(s.technology.last_index);
  auto const &technology{s.technology};
  auto write=[&](technology_entry const &t){file.integer(t.early);file.integer(t.late);file.integer(t.available);};
  for(size_t i{};i<technology.chassis_count;++i)write(technology.entries.at(i));
  for(size_t i{};i<technology.weapon_count;++i)write(technology.entries.at(technology.chassis_slots+i));
  file.integer(s.budget);write_words(file,s.counters);file.integer(s.repair_policy);return std::move(file).finish();
}
career_record read_career(bytes source,game_edition edition) {
  binary_reader file{source,"career"};career_record out;out.edition=edition;out.active=file.integer<uint8_t>();out.mode=file.integer<int16_t>();read_words(file,out.names);
  for(size_t i{};i<out.states.size();++i){out.states[i]=read_career_state(file,edition,edition==game_edition::expansion?out.mode:0);out.inventories[i]=read_armory(file);if(edition==game_edition::expansion&&out.mode)out.inventories[i].resize(22);}
  file.require_end();return out;
}
std::vector<uint8_t> write_career(career_record const &career) {
  binary_writer file;file.integer(career.active);file.integer(career.mode);write_words(file,career.names);
  for(size_t i{};i<career.states.size();++i){file.append(write_career_state(career.states[i]));file.append(write_armory(career.inventories[i]));}
  return std::move(file).finish();
}
void checkpoint_career(career_record &career,size_t history,int16_t progress,int32_t budget,std::array<int16_t,100> const &counters,int16_t policy) {
  if(history>=career.states.size())throw std::out_of_range{"career history slot"};
  auto &state{career.states[0]};state.valid=1;state.node_progress=progress;state.budget=budget;state.counters=counters;state.repair_policy=policy;
  if(history){career.states[history]=state;career.inventories[history]=career.inventories[0];for(auto &stack:career.inventories[history])std::ranges::reverse(stack);}
}
} // namespace earthsiege
