#pragma once
#include "simulation/random_state.h"
#include <span>
#include <vector>
namespace earthsiege {
struct music_contact {int32_t distance{};uint8_t side{};bool disabled{},dead{},waiting{},visible{};};
struct combat_music_state {int16_t track{-1};bool battle{};uint32_t deadline{};bool fading{};};
struct music_playback {std::array<uint8_t,6> volume{100,100,100,100,100,100};std::array<bool,6> playing{};};
enum class music_action:uint8_t {load,play,stop,volume};
struct music_request {music_action action;uint8_t track;uint8_t value{};};
struct music_contacts {std::vector<music_contact> actors,flyers;};
auto advance_combat_music(combat_music_state &state,music_playback &playback,std::span<music_contact const> actors,std::span<music_contact const> flyers,uint32_t clock,bool enabled,bool driver,random_state &random)->std::vector<music_request>;
}
