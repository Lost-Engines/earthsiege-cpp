#include "music_setup.h"
#include <algorithm>
#include <array>
#include <string>
#include "window.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"

namespace earthsiege {
namespace {
struct panel {
  font face;
  palette colours;
  framebuffer frame;
  explicit panel(resource_store const &resources) : face{read_font(resources.read("simvol0", "dfn/green6x8.dfn"))} {
    for(auto &glyph : face.glyphs) for(auto &pixel : glyph.pixels) if(pixel) pixel = 1;
    colours.colours.fill(0xff101820);
    colours.colours[1] = 0xffd8e8df;
    colours.colours[2] = 0xff315b49;
  }
  void text(std::string_view value, int x, int y) { frame.text(face, value, x, y + face.baseline); }
  void present(window &display) {
    auto const p{display.pointer_position()};
    frame.line(p[0] - 3, p[1], p[0] + 3, p[1], 1);
    frame.line(p[0], p[1] - 3, p[0], p[1] + 3, 1);
    display.present(frame, colours);
  }
};
constexpr std::array choices{music_device::opl2, music_device::opl3, music_device::sc55,
  music_device::scc1a, music_device::mt32, music_device::mt32_mpu, music_device::soundfont};
constexpr std::array labels{"AdLib / Sound Blaster (OPL2)", "Sound Blaster Pro / 16 (OPL3)",
  "Sound Canvas SC-55", "Sound Canvas SCC-1A", "MT-32 / CM-32L: original routing",
  "MT-32 / CM-32L: MPU-401 experiment", "General MIDI SoundFont"};
} // namespace

auto choose_game_edition(window &display, resource_store const &resources)->std::optional<game_edition> {
  panel ui{resources};int selected{};
  while(!glfwWindowShouldClose(display.get())){
    window_event event;
    while(display.poll(event)){
      if(event.type==event_type::close)return {};
      if(event.type==event_type::key_down&&!event.repeat){
        if(event.key==GLFW_KEY_ESCAPE)return {};
        if(event.key==GLFW_KEY_UP||event.key==GLFW_KEY_DOWN)selected=1-selected;
        if(event.key==GLFW_KEY_ENTER||event.key==GLFW_KEY_KP_ENTER)return selected?game_edition::expansion:game_edition::original;
        if(event.key==GLFW_KEY_1)return game_edition::original;
        if(event.key==GLFW_KEY_2)return game_edition::expansion;
      }
      bool const inside{event.x>=10&&event.x<310&&event.y>=66&&event.y<110};
      if(event.type==event_type::motion&&inside)selected=(event.y-66)/22;
      if(event.type==event_type::button_down&&event.button==GLFW_MOUSE_BUTTON_LEFT&&inside)return (event.y-66)/22?game_edition::expansion:game_edition::original;
    }
    ui.frame.clear();ui.text("EARTHSIEGE",10,15);ui.text("Choose a game",10,37);
    for(int i{};i<2;++i){int y{66+i*22};if(selected==i)ui.frame.fill({10,y,309,y+19},2);ui.text(i?"2. Earthsiege Expansion":"1. Earthsiege",16,y+6);}
    ui.text("Each edition keeps separate saved games.",10,139);
    ui.text("Click or 1-2: choose   Esc: quit",10,176);ui.present(display);glfwWaitEventsTimeout(.01);
  }
  return {};
}
auto choose_music_device(window &display, resource_store const &resources, music_device current)->std::optional<music_device> {
  panel ui{resources};
  auto const found{std::ranges::find(choices, current)};
  int selected{found == choices.end() ? 1 : static_cast<int>(found - choices.begin())};
  while(!glfwWindowShouldClose(display.get())) {
    window_event event;
    while(display.poll(event)) {
      if(event.type == event_type::close) return {};
      if(event.type == event_type::key_down && !event.repeat) {
        if(event.key == GLFW_KEY_ESCAPE) return {};
        if(event.key == GLFW_KEY_UP) selected = (selected + 6) % 7;
        if(event.key == GLFW_KEY_DOWN) selected = (selected + 1) % 7;
        if(event.key == GLFW_KEY_ENTER || event.key == GLFW_KEY_KP_ENTER) return choices[static_cast<size_t>(selected)];
        if(event.key >= GLFW_KEY_1 && event.key <= GLFW_KEY_7) return choices[static_cast<size_t>(event.key - GLFW_KEY_1)];
      }
      if(event.type == event_type::motion && event.x >= 6 && event.x < 314 && event.y >= 27 && event.y < 125) selected = (event.y - 27) / 14;
      if(event.type == event_type::button_down && event.button == GLFW_MOUSE_BUTTON_LEFT
        && event.x >= 6 && event.x < 314 && event.y >= 27 && event.y < 125) return choices[static_cast<size_t>((event.y - 27) / 14)];
    }
    ui.frame.clear();
    ui.text("MUSIC HARDWARE", 10, 7);
    for(size_t i{}; i < choices.size(); ++i) {
      int const y{27 + static_cast<int>(i) * 14};
      if(static_cast<int>(i) == selected) ui.frame.fill({6, y, 313, y + 12}, 2);
      ui.text(std::to_string(i + 1) + ". " + labels[i], 10, y + 2);
      if(choices[i] == current) ui.text("*", 302, y + 2);
    }
    if(selected == 4) {
      ui.text("Original MT-32 routing has no melodic tracks.", 6, 136);
      ui.text("Only sampled percussion may play.", 6, 147);
    } else if(selected == 5) {
      ui.text("Sends General MIDI parts to the MT-32.", 6, 136);
      ui.text("Experimental: instrument numbering differs.", 6, 147);
    } else if(selected == 2 || selected == 3) {
      ui.text("Requires ROMs: run scripts/fetch-assets.", 6, 136);
    } else if(selected == 6) {
      ui.text("Requires a SoundFont; see docs/MUSIC.md.", 6, 136);
    }
    ui.text("Selection restarts the song and is saved.", 6, 163);
    ui.text("Click or 1-7: choose   Esc: cancel", 6, 181);
    ui.present(display);
    glfwWaitEventsTimeout(.01);
  }
  return {};
}
void show_music_error(window &display, resource_store const &resources, std::string_view message) {
  panel ui{resources};
  while(!glfwWindowShouldClose(display.get())) {
    window_event event;
    while(display.poll(event)) {
      if(event.type == event_type::close || (event.type == event_type::key_down && !event.repeat
        && (event.key == GLFW_KEY_ESCAPE || event.key == GLFW_KEY_ENTER)) || event.type == event_type::button_up) return;
    }
    ui.frame.clear();
    ui.text("MUSIC DEVICE UNAVAILABLE", 8, 8);
    size_t at{};
    for(int y{28}; at < message.size() && y < 172; y += 11) {
      size_t end{std::min(at + 49, message.size())};
      auto const newline{message.find('\n', at)};
      if(newline < end) end = newline;
      ui.text(message.substr(at, end - at), 6, y);
      at = end + (end < message.size() && message[end] == '\n' ? 1 : 0);
    }
    ui.text("Enter / Esc / click: return", 6, 183);
    ui.present(display);
    glfwWaitEventsTimeout(.01);
  }
}

} // namespace earthsiege
