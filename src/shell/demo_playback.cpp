#include "demo_playback.h"
#include <format>
#include <stdexcept>
#include <utility>

namespace earthsiege {
namespace {
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
  calibrations = load_host_calibrations(saves);
  view_.configure_input(settings, calibrations);
  view_.begin_startup();
}
tape_frame const &demo_playback::poll(bool static_dialog) {
  auto const index{next++};
  auto const &record{tape.frames[index]};
  for(auto const &event : record.mouse) pointer = event.position;
  if(!packet_periods.empty()) {
    pending_ms += packet_periods[index];
  } else if(!static_dialog) {
    // A replacement tape has no DOSBox timing capture. Use its nominal
    // simulation duration for moving scenes. DOS polls a static alert freely.
    nominal_fraction += record.delta * 100u;
    pending_ms += nominal_fraction / 256;
    nominal_fraction %= 256;
  }
  return record;
}
void demo_playback::advance_clock(unsigned milliseconds) {
  elapsed_ms += milliseconds;
  view_.advance_time(milliseconds);
}
bool demo_playback::present(presenter const &show, framebuffer const &frame) {
  if(packet_periods.empty()) pending_ms += static_cast<unsigned>(view_.pan_frame_count() * 1000 / 70);
  auto const milliseconds{std::exchange(pending_ms, 0u)};
  advance_clock(milliseconds);
  if(!show(milliseconds, frame)) finished = true;
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
  poll(true);
  ++modal_packets;
  if(auto const warp{dialog.take_warp()}) pointer = *warp;
  suppressed = true;
  bool left{false};
  std::optional<int> response;
  while(!response && !finished) {
    if(next == tape.frames.size()) {
      finished = true;
      break;
    }
    auto const &input{poll(true)};
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
    present(show, frame);
  }
  if(response && dialog.exits_mission(*response)) finished = true;
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
  bool left{false};
  bool right{false};
  while(!options.done() && !finished) {
    if(next == tape.frames.size()) {
      finished = true;
      break;
    }
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
    present(show, frame);
  }
  view_.apply_preferences(settings);
  view_.end_options();
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
    if(!show(milliseconds, view_.image())) finished = true;
    return;
  }
  bool const eof{next == tape.frames.size()};
  tape_frame empty;
  empty.delta = last_delta;
  auto const &record{eof ? empty : poll()};
  last_delta = record.delta;
  bool const complete{view_.step_recorded(record, static_cast<uint32_t>(elapsed_ms * 60 / 1000), suppressed,
    {
      .command{[&](uint16_t code){return command(code, show);}},
     .outcome{[&](mission_status status){return alert(static_cast<int>(status), show);}}
    })};
  ++simulation_frames;
  if(complete && !finished) {
    view_.draw();
    present(show, view_.image());
  }
  // Original playback performs one zero-input update at EOF, retaining ratio.
  if(eof || !complete) finished = true;
}
}
