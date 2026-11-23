#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "shell/demo_playback.h"
#include "shell/game_shell.h"
using earthsiege::demo_playback;
using earthsiege::framebuffer;
using earthsiege::game_shell;
using earthsiege::resource_store;
using earthsiege::save_store;
namespace {
void require(bool ok, char const *why) {
  if(!ok) throw std::runtime_error{why};
}
struct recording_span {
  size_t first_poll, last_poll;
  unsigned earliest_ticks;
  unsigned latest_ticks{earliest_ticks};
  uint64_t duration{0};
  void observe(size_t poll, unsigned milliseconds) {
    if(first_poll <= poll && poll < last_poll) duration += milliseconds;
  }
  void check() const {
    require(duration >= earliest_ticks * 1000u / 120 && duration <= (latest_ticks * 1000u + 119) / 120,
            "presented interval disagrees with the recording clock");
  }
};
struct cleanup {
  std::filesystem::path path;
  ~cleanup() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
};
void click(game_shell &shell, uint16_t id) {
  auto const *control{shell.current_dialog().find(id)};
  require(control && control->enabled, "enabled menu control");
  int const x{(control->bounds.left + control->bounds.right) / 2};
  int const y{(control->bounds.top + control->bounds.bottom) / 2};
  shell.mouse_down(x, y);
  shell.mouse_up(x, y);
  for(unsigned i{0}; shell.transition_active() && i < 100; ++i) shell.advance(20);
}
} // anonymous namespace
int main(int argc, char **argv) try {
  if(argc < 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  cleanup temp{std::filesystem::path{argv[2]} / ("options-demo-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
  game_shell shell{resources, temp.path};
  click(shell, 1300);
  click(shell, 1310);
  require(shell.current_dialog().selection(1316) == 3, "255 detail values display MAX, not HIGH");
  auto const initial{shell.settings().current()};
  // GO's four stored presets, followed through the actual mouse button.
  for(int level{0}; level != 4; ++level) {
    click(shell, 1313);
    require(shell.current_dialog().selection(1316) == level, "all four detail presets are reachable");
    auto const &v{shell.settings().current()};
    require(v[3] == (level != 0) && v[4] == 63 + 64 * level && v[5] == v[4] && v[6] == v[4], "original preset bytes");
  }
  click(shell, 1313);
  click(shell, 99);
  click(shell, 1310);
  require(shell.settings().current() == initial && shell.current_dialog().selection(1316) == 3, "Cancel restores values and label");
  click(shell, 1313);
  click(shell, 100);
  click(shell, 1310);
  require(shell.current_dialog().selection(1316) == 0, "accepted preset retained when reopening");
  click(shell, 1313);
  click(shell, 100);
  click(shell, 1310);
  require(shell.current_dialog().selection(1316) == 1, "MEDIUM retains its separate shell selector");
  // Put the original maximum values back for replay and persistence checks.
  click(shell, 1313);
  click(shell, 1313);
  click(shell, 100);
  game_shell restored{resources, temp.path};
  click(restored, 1300);
  click(restored, 1310);
  require(restored.current_dialog().selection(1316) == 3, "MAX survives restart");
  click(shell, 1301);
  require(shell.take_demo_request() && !shell.take_demo_request() && !shell.ready_to_launch(), "View Demo requests playback once, not a career launch");
  shell.demo_return();
  require(shell.screen() == 1000, "demo returns to main menu");
  auto const full{argc > 3};
  for(unsigned demo{argc > 4 ? static_cast<unsigned>(std::stoul(argv[4])) : 1}; demo <= 4; ++demo) {
    demo_playback playback{resources, demo, shell.settings(), temp.path};
    size_t shown{0};
    uint64_t milliseconds{0};
    uint64_t stereo_frames{0};
    bool checked_music_cursor{false};
    bool const apocalypse_panel{resources.edition() == earthsiege::game_edition::expansion && demo == 1};
    bool checked_panel{false};
    unsigned timed_pans{0};
    bool const retail_patrol{resources.edition() == earthsiege::game_edition::original && demo == 3};
    unsigned const interrupt_frame{retail_patrol ? 481U : 3U};
    std::vector<recording_span> spans;
    if(retail_patrol) {
      // The last span is in the exit dialog. Its mouse moves from tick 9294
      // to 9342 (48 slow ticks), while DOSBox took only 10 ms to poll it.
      spans = {{5, 55, 370}, {200, 300, 1085}, {2227, 2470, 96}};
    }
    // Independent mouse timestamps span 1,634 and 1,506 ticks at 60 Hz.
    if(resources.edition() == earthsiege::game_edition::original && demo == 1) spans = {{1476, 1802, 3267}};
    if(resources.edition() == earthsiege::game_edition::expansion && demo == 3) spans = {{2989, 3216, 3011}};
    if(resources.edition() == earthsiege::game_edition::expansion && demo == 4) {
      // Mouse events bracket the poll to within its own world interval.
      // These two stretches contain 156 and 253 upper-clipped steps.
      spans = {{1597, 2186, 9579, 9602}, {629, 3304, 39857, 39875}};
    }
    while(!playback.done()) {
      playback.step([&](unsigned period, framebuffer const &frame, framebuffer const *erased){
        milliseconds += period;
        for(auto &span : spans) span.observe(playback.packets_read(), period);
        if(retail_patrol && playback.packets_read() == 1) {
          require(period == 83, "first patrol frame holds for the ten ticks stored in packet two");
        }
        if(retail_patrol && (playback.packets_read() == 1 || playback.packets_read() == 60)) {
          require(erased == &playback.view().image(), "cursor must leave the retained cockpit page clean");
          auto expected{*erased};
          earthsiege::software_cursor{resources.read("simvol0", "dci/PCURSOR.DCI")}.draw(expected, 160, 72);
          require(frame.pixels() == expected.pixels(), "DOS keeps the green diamond until the first side pan");
        }
        if(retail_patrol && playback.frames_played() == 301) require(!erased, "external camera hides the cockpit cursor");
        if(retail_patrol && playback.frames_played() == 481) {
          require(erased == &playback.view().image(), "forward cockpit supplies the clean phase after a side pan");
          auto expected{*erased};
          // Retail DOS frame 480 differs from its saved-under page by this exact 13-pixel plus.
          for(int i{-3}; i <= 3; ++i) {
            expected.pixel(160 + i, 72, 60);
            expected.pixel(160, 72 + i, 60);
          }
          require(frame.pixels() == expected.pixels() && frame.pixels() != erased->pixels(), "white DOS cursor over the green gunsight");
        }
        stereo_frames += playback.view().take_audio().size() / 2;
        require(stereo_frames == milliseconds * 120 / 1000 * 44100 / 120, "continuous 44.1 kHz audio on the presentation clock");
        auto pans{playback.view().take_pan_frames()};
        if(resources.edition() == earthsiege::game_edition::original && demo == 1 && playback.packets_read() == 1) {
          require(erased, "retail demo 1 cockpit cursor is visible");
          auto expected{*erased};
          earthsiege::software_cursor{resources.read("simvol0", "dci/CURSOR.DCI")}.draw(expected, 192, 144);
          require(frame.pixels() == expected.pixels(), "first recorded mouse movement takes the cursor off the gunsight");
        }
        if(retail_patrol && !pans.empty()) {
          require(period >= 500, "cockpit pan has audio for its complete VGA scroll");
          ++timed_pans;
        }
        if(retail_patrol && playback.packets_read() == 970) {
          earthsiege::software_cursor arrow{resources.read("simvol0", "dci/ALERT.DCI")};
          auto stamped{frame};
          arrow.draw(stamped, 51, 148);
          require(stamped.pixels() == frame.pixels(), "DOS music button cursor at 51,148");
          checked_music_cursor = true;
        }
        if(retail_patrol && playback.frames_played() == 1021) {
          require(erased, "cockpit cursor restored after the music dialog");
          auto expected{*erased};
          earthsiege::software_cursor{resources.read("simvol0", "dci/CURSOR.DCI")}.draw(expected, 0, 0);
          require(frame.pixels() == expected.pixels(), "dialog restores the tape's raw mouse position");
        }
        require(frame.pixels().size() == 320 * 200, "demo presents game frames");
        ++shown;
        if(apocalypse_panel && shown == 90) {
          // Native-resolution RGB crop of the DOSBox lower panel, including the F7/F8 buttons and crew.
          std::vector<uint8_t> rgb;
          auto const &palette{playback.view().colours().colours};
          for(int y{124}; y < 193; ++y) {
            for(int x{29}; x < 294; ++x) {
              auto colour{palette[frame.pixels()[static_cast<size_t>(y * earthsiege::screen_width + x)]]};
              for(unsigned shift : {16U, 8U, 0U}) rgb.push_back(static_cast<uint8_t>(colour >> shift));
            }
          }
          require(earthsiege::crc32(rgb) == 0x4f79198a, "Apocalypse crew and lower panel differ from DOS");
          checked_panel = true;
        }
        return full || playback.frames_played() < interrupt_frame || (apocalypse_panel && !checked_panel);
      });
      require(shown < 20000, "playback terminates");
    }
    std::cout << "Completed demo " << demo << ": " << playback.frames_played() << " frames, " << playback.modal_packets_read() << " modal packets, " << playback.packets_read() << '/' << playback.packet_count() << " packets, " << shown << " presentations, " << milliseconds << " ms" << std::endl;
    if(apocalypse_panel) require(checked_panel, "expansion demo shows the lower cockpit");
    if(retail_patrol) {
      // A user-supplied tape has no matching DOSBox clock file.
      demo_playback uncaptured{resources, demo, shell.settings(), temp.path, {}, temp.path / "no-clocks"};
      recording_span opening{5, 55, 370};
      while(!uncaptured.done()) {
        uncaptured.step([&](unsigned period, framebuffer const &, framebuffer const *){
          uncaptured.view().take_audio();
          uncaptured.view().take_pan_frames();
          opening.observe(uncaptured.packets_read(), period);
          return uncaptured.packets_read() < 55;
        });
      }
      opening.check();
    }
    if(full) {
      require(playback.packets_read() == playback.packet_count(), "complete recording consumed");
      require(playback.frames_played() > 1000, "full demo simulates");
      for(auto const &span : spans) span.check();
      if(retail_patrol) {
        require(checked_music_cursor && timed_pans == 4, "retail patrol music menu and four cockpit pans");
      }
    } else {
      require((apocalypse_panel || playback.frames_played() == interrupt_frame) && playback.packets_read() < playback.packet_count(), "host input can interrupt playback");
    }
  }
  require(save_store{temp.path, resources.edition()}.index().entries.empty(), "playback creates no career");
  std::cout << "Options presets, persistence, demo launch, playback and return pass\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
