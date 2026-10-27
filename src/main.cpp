#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <SDL.h>
#include "platform/window.h"
#include "resources/resource_store.h"
#include "shell/dialog.h"
#include "shell/mission_view.h"

namespace {

auto integer_option(std::string_view text)->unsigned int {
  unsigned int result{};
  auto const [end, error]{std::from_chars(text.data(), text.data() + text.size(), result)};
  if(error != std::errc{} || end != text.data() + text.size()) throw std::runtime_error{"invalid integer option: " + std::string{text}};
  return result;
}

auto held_key(SDL_Keycode const code)->std::optional<earthsiege::held_key> {
  using enum earthsiege::held_key;
  switch(code) {
    case SDLK_KP_7: return forward_left;
    case SDLK_KP_8: case SDLK_UP: return forward;
    case SDLK_KP_9: return forward_right;
    case SDLK_KP_4: case SDLK_LEFT: return left;
    case SDLK_KP_6: case SDLK_RIGHT: return right;
    case SDLK_KP_1: return reverse_left;
    case SDLK_KP_2: case SDLK_DOWN: return reverse;
    case SDLK_KP_3: return reverse_right;
    case SDLK_m: return aim_down;
    case SDLK_j: return aim_left;
    case SDLK_k: return aim_right;
    case SDLK_i: return aim_up;
    case SDLK_SPACE: return fire;
    case SDLK_RETURN: return enter;
    default: return std::nullopt;
  }
}

} // namespace

auto main(int argc, char *argv[])->int try {
  std::filesystem::path data_dir{earthsiege::resource_store::locate(std::filesystem::absolute(argv[0]))};
  std::filesystem::path screenshot;
  bool list{};
  bool headless{};
  unsigned int scale{};
  std::optional<unsigned int> inspect_demo;
  unsigned int frame_limit{};
  for(int index{1}; index < argc; ++index) {
    std::string_view const argument{argv[index]};
    if(argument == "--help") {
      std::cout << "Earthsiege C++\nUsage: earthsiege [options]\n"
        "  --data-dir PATH       Original game data (default: bundled assets)\n"
        "  --inspect-demo N     Inspect demo 1–4 terrain and player roster (not replay)\n"
        "  --scale N             Integer window scale (default: largest fit)\n"
        "  --screenshot PATH     Save an unscaled PPM frame\n"
        "  --frames N            Exit after N frames\n"
        "  --headless            Render a frame without creating a window\n"
        "  --list-resources      Print all original resource sizes and CRCs\n";
      return EXIT_SUCCESS;
    }
    if(argument == "--data-dir" && index + 1 < argc) data_dir = argv[++index];
    else if(argument == "--screenshot" && index + 1 < argc) screenshot = argv[++index];
    else if(argument == "--inspect-demo" && index + 1 < argc) inspect_demo = integer_option(argv[++index]);
    else if(argument == "--scale" && index + 1 < argc) scale = integer_option(argv[++index]);
    else if(argument == "--frames" && index + 1 < argc) frame_limit = integer_option(argv[++index]);
    else if(argument == "--headless") headless = true;
    else if(argument == "--list-resources") list = true;
    else throw std::runtime_error{"unknown or incomplete option: " + std::string{argument}};
  }
  earthsiege::resource_store const resources{data_dir};
  if(list) {
    for(auto const &[volume, archive] : resources.archives()) {
      for(auto const &[name, entry] : archive.entries()) {
        std::cout << volume << ':' << name << '\t' << entry.size << '\t' << std::hex << std::setw(8) << std::setfill('0') << earthsiege::crc32(archive.read(entry)) << std::dec << '\n';
      }
    }
    return EXIT_SUCCESS;
  }
  earthsiege::dialog menu{resources};
  earthsiege::framebuffer menu_frame;
  std::unique_ptr<earthsiege::mission_view> scene;
  if(inspect_demo) {
    scene = std::make_unique<earthsiege::mission_view>(resources, *inspect_demo);
    std::cout << "Scene inspection: arrows/keypad walk and turn; I/J/K/M aim; Alt increases aiming speed.\n"
      "Ctrl + arrows move camera; Ctrl + Shift + up/down change distance.\n"
      "F1 attached, F2 orbit, F3 free camera; P pauses, Escape returns to menu.\n"
      "Combat, AI and cockpit are not yet active.\n";
  }
  std::unique_ptr<earthsiege::window> display;
  if(!headless) display = std::make_unique<earthsiege::window>(scale);
  else if(!frame_limit) frame_limit = 1;
  bool running{true};
  bool focused{true};
  unsigned int frames{};
  uint64_t previous{display ? SDL_GetTicks64() : 0};
  uint64_t pending{0};
  while(running) {
    SDL_Event event{};
    while(display && display->poll(event)) {
      if(event.type == SDL_QUIT) running = false;
      if(event.type == SDL_WINDOWEVENT) {
        if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          focused = false;
          menu.cancel_press();
          if(scene) scene->clear_input();
        }
        if(event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) focused = true;
      }
      if(event.type == SDL_KEYDOWN && !event.key.repeat) {
        if(event.key.keysym.sym == SDLK_ESCAPE) {
          if(scene) scene.reset();
          else running = false;
        } else if(scene && focused) {
          if(event.key.keysym.sym == SDLK_p) scene->toggle_pause();
          if(event.key.keysym.sym == SDLK_F1) scene->change_camera(earthsiege::camera_mode::attached);
          if(event.key.keysym.sym == SDLK_F2) scene->change_camera(earthsiege::camera_mode::orbit);
          if(event.key.keysym.sym == SDLK_F3) scene->change_camera(earthsiege::camera_mode::free);
        }
      }
      if(scene) {
        if(focused && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && !event.key.repeat) {
          if(auto const key{held_key(event.key.keysym.sym)}) scene->key(*key, event.type == SDL_KEYDOWN, (event.key.keysym.mod & KMOD_ALT) != 0);
        }
      } else if(focused) {
        if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) menu.mouse_down(event.button.x, event.button.y);
        if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
          auto const action{menu.mouse_up(event.button.x, event.button.y)};
          if(action == 1003) running = false;
          else if(action) std::cout << "Menu action " << *action << std::endl;
        }
      }
    }
    auto const now{display ? SDL_GetTicks64() : previous + 25};
    pending += focused ? std::min<uint64_t>(now - previous, 100) : 0;
    previous = now;
    if(!focused) pending = 0;
    if(scene) {
      if(frames) while(pending >= 25) {
        auto const modifiers{display ? SDL_GetModState() : KMOD_NONE};
        scene->step((modifiers & KMOD_CTRL) != 0, (modifiers & KMOD_SHIFT) != 0);
        pending -= 25;
      }
      else pending = 0;
      scene->draw();
    } else menu.draw(menu_frame);
    auto const &frame{scene ? scene->image() : menu_frame};
    auto const &colours{scene ? scene->colours() : menu.current_palette()};
    if(display) display->present(frame, colours);
    ++frames;
    if(frame_limit && frames >= frame_limit) running = false;
    if(display) SDL_Delay(8);
  }
  if(!screenshot.empty()) {
    if(scene) scene->image().save(screenshot, scene->colours());
    else menu_frame.save(screenshot, menu.current_palette());
  }
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
