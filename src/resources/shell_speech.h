#pragma once
#include "resources/binary_reader.h"
namespace earthsiege {
struct shell_speech_clip {
  std::string sync;
  std::string sample;
};
struct shell_speech_group {
  std::string program;
  std::vector<shell_speech_clip> clips;
};
shell_speech_group read_shell_speech_group(bytes);
struct shell_mouth_frame {
  uint16_t page{0};
  uint16_t deadline{0};
};
std::vector<shell_mouth_frame> read_shell_mouth_timeline(bytes);
/// GO's cumulative 16-bit timeline, distinct from the simulator's SNC format.
class shell_mouth {
  std::vector<shell_mouth_frame> frames;
  size_t cursor_{0};
  uint16_t clock_{0};
  uint16_t page_{1};
  uint16_t program_pages{0};
  bool active_{true};
  bool pending_{false};
public:
  shell_mouth(std::vector<shell_mouth_frame>, uint16_t pages);
  void update(uint16_t clock);
  size_t cursor() const {
    return cursor_;
  }
  uint16_t page() const {
    return page_;
  }
  bool active() const {
    return active_;
  }
  bool pending_stop() const {
    return pending_;
  }
  void stop();
};
} // namespace earthsiege
