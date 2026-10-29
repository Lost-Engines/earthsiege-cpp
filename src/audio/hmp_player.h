#pragma once
#include <optional>
#include "resources/hmp.h"
namespace earthsiege {
struct midi_packet {
  uint32_t device{};
  std::vector<uint8_t> wire;
  std::optional<uint32_t> source_offset;
};
/// Original HMI routing and timer scheduler, without game-installed branch callbacks.
class hmp_player {
  struct track_state {
    uint32_t cursor{}, elapsed{}, delta{};
    std::optional<size_t> device;
    bool active{};
    std::vector<uint8_t> loops;
  };
  hmp_song const &song;
  std::vector<uint32_t> devices;
  std::vector<track_state> tracks;
  bool active{true};
  size_t remaining{};
  size_t branch(size_t track,uint8_t id)const;
  void read_delta(size_t track);
  void jump(size_t track,size_t branch,bool global,std::vector<midi_packet> &packets);
public:
  hmp_player(hmp_song const &song,std::span<uint32_t const> installed);
  void restart();
  void stop();
  bool playing()const;
  std::vector<std::optional<size_t>> routing()const;
  std::vector<midi_packet> tick();
};
} // namespace earthsiege
