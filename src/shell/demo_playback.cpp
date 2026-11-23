#include "demo_playback.h"
#include <algorithm>
#include <format>
#include <span>
#include <stdexcept>
#include <utility>

namespace earthsiege {
namespace {
// Mouse batches carry a 60 Hz timestamp even while the world is frozen.
// Spread the time between two batches over the intervening empty input polls.
struct dialog_clock {
  size_t polls_left{0};
  uint64_t ms_left{0};

  unsigned hold(std::span<tape_frame const> packets, bool closed, unsigned fallback) {
    if(closed) return fallback;
    if(!polls_left && !packets.front().mouse.empty()) {
      auto const start{packets.front().mouse.back().timestamp};
      for(size_t i{1}; i < packets.size(); ++i) {
        auto const &packet{packets[i]};
        if(!packet.mouse.empty()) {
          auto const ticks{packet.mouse.back().timestamp - start};
          auto const duration{(uint64_t{ticks} * 1000 + uint64_t{start} * 1000 % 60) / 60};
          // Reject discontinuous clocks outside the range of our poll intervals.
          if(duration <= uint64_t{UINT16_MAX} * i) {
            polls_left = i;
            ms_left = duration;
          }
          break;
        }
        if(packet.command || packet.buttons || !packet.commands.empty()) break;
      }
    }
    if(!polls_left) return fallback;
    auto const interval{static_cast<unsigned>(ms_left / polls_left)};
    ms_left -= interval;
    --polls_left;
    return interval;
  }
};
input_tape load_demo(resource_store const &resources, unsigned demo, std::filesystem::path clocks,
                     std::vector<uint16_t> &startup, std::vector<uint16_t> &packets) {
  if(demo < 1 || demo > 4) throw std::runtime_error{"demo must be between 1 and 4"};
  auto const name{"TAPES/DEMO" + std::to_string(demo) + ".TAP"};
  auto const path{resources.external(name)};
  if(!path) throw std::runtime_error{"Missing demo recording: " + name};
  auto const data{read_file(*path)};
  auto tape{read_input_tape(data, resources.edition())};
  if(clocks.empty()) {
    auto root{resources.root()};
    if(resources.edition() == game_edition::expansion) root = root.parent_path();
    clocks = root / "demo-clocks";
  }
  auto const clock{clocks / std::format("{:08x}.clk", crc32(data))};
  if(std::filesystem::is_regular_file(clock)) {
    auto const raw{read_file(clock)};
    binary_reader file{raw, clock.string()};
    auto const count{file.integer<uint16_t>()};
    if(file.remaining() != 2 * (count + tape.frames.size())) file.fail("demo clock length");
    for(unsigned i{0}; i < count; ++i) startup.push_back(file.integer<uint16_t>());
    while(file.remaining()) packets.push_back(file.integer<uint16_t>());
  }
  return tape;
}
}
demo_playback::demo_playback(resource_store const &r, unsigned demo, preferences const &prefs,
                             std::filesystem::path save_directory, music_settings const &music,
                             std::filesystem::path const &clock_directory)
 : resources{r},
   tape{load_demo(r, demo, clock_directory, startup_periods, packet_periods)},
   dialog_cursor{r.read("simvol0", "dci/ALERT.DCI")},
   view_{r, tape, music},
   settings{prefs},
   saves{std::move(save_directory), r.edition()} {
  pointer = view_.initial_pointer();
  calibrations = load_host_calibrations(saves);
  view_.configure_input(settings, calibrations);
  view_.begin_startup();
}
tape_frame const &demo_playback::poll(bool cursor_visible) {
  auto const index{next++};
  auto const &record{tape.frames[index]};
  for(auto const &event : record.mouse) {
    // 444d0 moves the displayed sprite on a changed position while visible.
    if(cursor_visible && event.position != last_mouse_event) pointer = event.position;
    last_mouse_event = event.position;
  }
  if(!packet_periods.empty()) capture_ms += packet_periods[index];
  return record;
}
unsigned demo_playback::world_hold(uint64_t poll_time) {
  auto const pan_ms{static_cast<unsigned>(view_.pan_frame_count() * 1000 / vga_refresh_hz)};
  if(next == tape.frames.size()) return std::max(capture_ms, pan_ms);
  // The writer samples elapsed time before each world update. The interval
  // after this update is therefore in the following primary input packet.
  auto const ratio{tape.frames[next].delta};
  auto const ticks{(ratio * nominal_frame_ticks + frame_ratio_one - 1u) / frame_ratio_one};
  tick_fraction += ticks * 1000;
  auto const period{tick_fraction / simulation_timer_hz};
  tick_fraction %= simulation_timer_hz;
  // A dialog inside this update has already spent part of the interval.
  auto const spent{elapsed_ms - poll_time};
  unsigned const remaining{period > spent ? static_cast<unsigned>(period - spent) : 0u};
  if(ratio == maximum_frame_ratio) {
    // Clipping lost the upper bound. Keep the time for any pending VGA scroll
    // and the estimate for this interval from the bundled timing data.
    return std::max({remaining, capture_ms, pan_ms});
  }
  return remaining;
}
void demo_playback::advance_clock(unsigned milliseconds) {
  elapsed_ms += milliseconds;
  view_.advance_time(milliseconds);
}
bool demo_playback::present(presenter const &show, framebuffer const &frame, unsigned milliseconds, framebuffer const *erased) {
  capture_ms = 0;
  advance_clock(milliseconds);
  if(!show(milliseconds, frame, erased)) finished = true;
  return !finished;
}
bool demo_playback::alert(int message, presenter const &show) {
  mission_alert dialog{message == -1 ? mission_alert{resources, view_.simulation().objective_text} : mission_alert{resources, message}};
  dialog.feedback([&]{view_.click_sound();});
  // 45f00 consumes an initialization poll before dispatching modal input.
  if(next == tape.frames.size()) {
    finished = true;
    return true;
  }
  poll();
  ++modal_packets;
  if(auto const warp{dialog.take_warp()}) pointer = *warp;
  suppressed = true;
  dialog_clock timing;
  bool left{false};
  std::optional<int> response;
  while(!response && !finished) {
    if(next == tape.frames.size()) {
      finished = true;
      break;
    }
    auto const entry_ms{capture_ms};
    auto const &input{poll()};
    ++modal_packets;
    for(auto const &event : input.mouse) {
      bool const held{bool(event.kind & 1)};
      if(held && !left) dialog.mouse_down(event.position.x, event.position.y);
      if(!held && left) response = dialog.mouse_up(event.position.x, event.position.y);
      left = held;
    }
    if(!response) response = dialog.input(input.command, input.buttons & 1, input.buttons & 2, settings.current()[10] != 0).response;
    auto frame{view_.image()};
    dialog.draw(frame);
    if(auto const warp{dialog.take_warp()}) pointer = *warp;
    dialog_cursor.draw(frame, pointer.x, pointer.y);
    auto const remaining{std::span{tape.frames}.subspan(next - 1)};
    present(show, frame, entry_ms + timing.hold(remaining, response.has_value(), capture_ms - entry_ms));
  }
  if(response && dialog.exits_mission(*response)) finished = true;
  // 460b8 restores the mouse fields saved from the primary input record.
  // Playback fills its mouse event queue separately and leaves those fields zero.
  pointer = {};
  return finished;
}
mission_command_result demo_playback::command(uint16_t code, presenter const &show) {
  using enum mission_command_result;
  if(code == 87 && view_.view_camera().mode == camera_mode::attached) return alert(-1, show) ? exit_mission : handled;
  if(code == 16) return alert(static_cast<int>(view_.status(true)), show) ? exit_mission : handled;
  if(code == 25 || code == 0x401 || code == 0x410) return alert(code == 25 ? 0 : 1, show) ? exit_mission : handled;
  if(code != 88 && code != 0x219) return unhandled;
  suppressed = true;
  view_.begin_options();
  if(next == tape.frames.size()) {
    finished = true;
    return exit_mission;
  }
  poll();
  ++modal_packets;
  mission_options_dialog options{resources, settings, saves, calibrations};
  options.feedback([&]{view_.click_sound();});
  if(auto const warp{options.take_warp()}) pointer = *warp;
  dialog_clock timing;
  bool left{false};
  bool right{false};
  while(!options.done() && !finished) {
    if(next == tape.frames.size()) {
      finished = true;
      break;
    }
    auto const entry_ms{capture_ms};
    auto const &input{poll()};
    ++modal_packets;
    options.key(input.command);
    view_.step_options_recorded();
    view_.draw();
    for(auto const &event : input.mouse) {
      bool const l{bool(event.kind & 1)};
      bool const r{bool(event.kind & 2)};
      if(l && !left) options.mouse_down(event.position.x, event.position.y);
      if(r && !right) options.mouse_down(event.position.x, event.position.y, true);
      if(!l && left) options.mouse_up(event.position.x, event.position.y);
      if(!r && right) options.mouse_up(event.position.x, event.position.y, true);
      left = l;
      right = r;
    }
    auto frame{view_.image()};
    options.draw(frame);
    if(auto const warp{options.take_warp()}) pointer = *warp;
    dialog_cursor.draw(frame, pointer.x, pointer.y);
    auto const remaining{std::span{tape.frames}.subspan(next - 1)};
    auto const fallback{packet_periods.empty() ? 1000 / vga_refresh_hz : capture_ms - entry_ms};
    present(show, frame, entry_ms + timing.hold(remaining, options.done(), fallback));
  }
  view_.apply_preferences(settings);
  view_.end_options();
  pointer = {};
  return finished ? exit_mission : handled;
}
void demo_playback::step(presenter const &show) {
  if(finished) return;
  if(view_.starting()) {
    unsigned milliseconds{17};
    if(startup_index + 1 < startup_periods.size()) {
      milliseconds = startup_periods[startup_index++];
      view_.draw_startup(static_cast<uint32_t>(elapsed_ms * 60 / 1000));
      advance_clock(milliseconds);
      auto const now{static_cast<uint32_t>(elapsed_ms * 60 / 1000)};
      view_.finish_startup_frame(now, now);
    } else {
      advance_clock(milliseconds);
      view_.step_startup();
    }
    if(!view_.starting()) {
      while(startup_index < startup_periods.size()) {
        auto const tail{startup_periods[startup_index++]};
        advance_clock(tail);
        milliseconds += tail;
      }
    }
    if(!show(milliseconds, view_.image(), nullptr)) finished = true;
    return;
  }
  auto const poll_time{elapsed_ms};
  bool const eof{next == tape.frames.size()};
  tape_frame empty;
  empty.delta = last_delta;
  auto const &record{eof ? empty : poll(view_.view_camera().mode == camera_mode::attached)};
  last_delta = record.delta;
  bool const complete{view_.step_recorded(record, static_cast<uint32_t>(elapsed_ms * 60 / 1000), suppressed,
    {
      .command{[&](uint16_t code){return command(code, show);}},
     .outcome{[&](mission_status status){return alert(static_cast<int>(status), show);}}
    })};
  ++simulation_frames;
  if(complete && !finished) {
    view_.draw();
    if(auto warp{view_.take_pointer_warp()}) pointer = *warp;
    auto const &clean{view_.image()};
    auto const hold{world_hold(poll_time)};
    if(auto const *cursor{view_.pointer_cursor(pointer.x, pointer.y)}) {
      auto marked{clean};
      cursor->draw(marked, pointer.x, pointer.y);
      present(show, marked, hold, &clean);
    } else {
      present(show, clean, hold);
    }
  }
  // Original playback performs one zero-input update at EOF, retaining ratio.
  if(eof || !complete) finished = true;
}
}
