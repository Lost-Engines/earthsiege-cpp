#pragma once
#include <functional>
#include <optional>
#include "resources/hmp.h"
namespace earthsiege {
struct midi_packet {
  uint32_t device{0};
  std::vector<uint8_t> wire;
  std::optional<uint32_t> source_offset;
};
/// Original HMI routing and timer scheduler, including the optional loop filter.
class hmp_player {
  struct track_state {
    uint32_t cursor{0};
    uint32_t elapsed{0};
    uint32_t delta{0};
    std::optional<size_t> device;
    bool active{false};
    std::vector<uint8_t> loops;
  };
  hmp_song const &song;
  std::vector<uint32_t> devices;
  std::vector<track_state> tracks;
  bool active{true};
  size_t remaining{0};
  size_t branch(size_t track, uint8_t id) const;
  void read_delta(size_t track);
  void jump(size_t track, size_t branch, bool global, std::vector<midi_packet> &packets);
public:
  std::function<bool(uint8_t)> loop_filter;
  hmp_player(hmp_song const &song, std::span<uint32_t const> installed);
  void restart();
  void stop();
  bool playing() const;
  std::vector<std::optional<size_t>> routing() const;
  std::vector<midi_packet> tick();
};
} // namespace earthsiege
