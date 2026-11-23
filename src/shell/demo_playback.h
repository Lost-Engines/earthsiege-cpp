#pragma once
#include "render/software_cursor.h"
#include "shell/mission_alert.h"
#include "shell/mission_options_dialog.h"
#include "shell/mission_view.h"

namespace earthsiege {
/// Attract-mode tape owner. Modal input is consumed at its original call site,
/// while the host presents frames and can interrupt playback without steering it.
class demo_playback {
public:
  using presenter = std::function<bool(unsigned milliseconds, framebuffer const &)>;
private:
  resource_store const &resources;
  std::vector<uint16_t> startup_periods;
  std::vector<uint16_t> packet_periods;
  input_tape tape;
  software_cursor dialog_cursor;
  vec2<int32_t> pointer{};
  size_t startup_index{0};
  mission_view view_;
  preferences settings;
  save_store saves;
  std::array<joystick_calibration, 2> calibrations{};
  size_t next{0};
  size_t simulation_frames{0};
  size_t modal_packets{0};
  uint64_t elapsed_ms{0};
  unsigned capture_ms{0};
  unsigned tick_fraction{0};
  uint16_t last_delta{0};
  bool finished{false};
  bool suppressed{false};
  tape_frame const &poll();
  unsigned world_hold(uint64_t poll_time);
  void advance_clock(unsigned milliseconds);
  bool present(presenter const &, framebuffer const &, unsigned milliseconds);
  bool alert(int message, presenter const &);
  mission_command_result command(uint16_t, presenter const &);
public:
  demo_playback(resource_store const &, unsigned demo, preferences const &,
                std::filesystem::path save_directory, music_settings const &music = {},
                std::filesystem::path const &clock_directory = {});
  mission_view &view() {
    return view_;
  }
  bool done() const {
    return finished;
  }
  size_t packets_read() const {
    return next;
  }
  size_t packet_count() const {
    return tape.frames.size();
  }
  size_t frames_played() const {
    return simulation_frames;
  }
  size_t modal_packets_read() const {
    return modal_packets;
  }
  void step(presenter const &);
};
}
