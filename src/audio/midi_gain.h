#pragma once
#include "hmp_player.h"
#include <array>
#include <utility>
namespace earthsiege {
// HMI's normal game path disables channel remapping. Per-packet CC7 gain and
// direct master/song updates intentionally use different integer expressions.
class midi_gain {
  std::vector<uint32_t> devices;
  std::vector<std::array<uint8_t,16>> volumes_;
  uint8_t master_{127};
public:
  explicit midi_gain(std::span<uint32_t const> installed);
  auto apply(midi_packet packet,uint8_t song_volume=127,bool muted=false)->midi_packet;
  auto set_master(uint8_t value)->std::vector<midi_packet>;
  auto set_song(uint8_t value,std::span<std::pair<size_t,uint8_t> const> active_tracks)const->std::vector<midi_packet>;
  auto volumes()const->decltype(volumes_)const&{return volumes_;}
  auto master()const->uint8_t{return master_;}
};
}
