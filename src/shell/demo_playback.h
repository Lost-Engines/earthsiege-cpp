#pragma once
#include "shell/mission_view.h"
#include "shell/mission_options_dialog.h"
#include "shell/mission_alert.h"

namespace earthsiege {
/// Attract-mode tape owner. Modal input is consumed at its original call site,
/// while the host presents frames and can interrupt playback without steering it.
class demo_playback {
public:
  using presenter = std::function<bool(unsigned milliseconds,framebuffer const &)>;
private:
  resource_store const &resources;
  input_tape tape;
  mission_view view_;
  preferences settings;
  save_store saves;
  std::array<joystick_calibration,2> calibrations{};
  size_t next{},simulation_frames{},modal_packets{};
  uint64_t elapsed_units{}; // milliseconds * 256, retaining fractional tape periods
  uint16_t last_delta{};
  bool finished{},suppressed{};
  unsigned advance(uint16_t delta);
  bool present(presenter const &,unsigned,framebuffer const &);
  bool alert(int message,presenter const &);
  mission_command_result command(uint16_t,presenter const &);
public:
  demo_playback(resource_store const &,unsigned demo,preferences const &,
                std::filesystem::path save_directory,music_settings const &music={});
  mission_view &view(){return view_;}
  bool done()const{return finished;}
  size_t packets_read()const{return next;}
  size_t packet_count()const{return tape.frames.size();}
  size_t frames_played()const{return simulation_frames;}
  size_t modal_packets_read()const{return modal_packets;}
  void step(presenter const &);
};
}
