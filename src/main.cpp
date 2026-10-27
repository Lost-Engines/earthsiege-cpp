#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <SDL.h>
#include "platform/window.h"
#include "resources/resource_store.h"
#include "shell/dialog.h"

namespace {

auto integer_option(std::string_view text)->unsigned int {
  unsigned int result{};
  auto const [end, error]{std::from_chars(text.data(), text.data() + text.size(), result)};
  if(error != std::errc{} || end != text.data() + text.size()) throw std::runtime_error{"invalid integer option: " + std::string{text}};
  return result;
}

} // namespace

auto main(int argc, char *argv[])->int try {
  std::filesystem::path data_dir{earthsiege::resource_store::locate(std::filesystem::absolute(argv[0]))};
  std::filesystem::path screenshot;
  bool list{};
  bool headless{};
  unsigned int scale{};
  unsigned int frame_limit{};
  for(int index{1}; index < argc; ++index) {
    std::string_view const argument{argv[index]};
    if(argument == "--help") {
      std::cout << "Earthsiege C++\nUsage: earthsiege [options]\n"
        "  --data-dir PATH       Original game data (default: bundled assets)\n"
        "  --scale N             Integer window scale (default: largest fit)\n"
        "  --screenshot PATH     Save an unscaled PPM frame\n"
        "  --frames N            Exit after N frames\n"
        "  --headless            Render a frame without creating a window\n"
        "  --list-resources      Print all original resource sizes and CRCs\n";
      return EXIT_SUCCESS;
    }
    if(argument == "--data-dir" && index + 1 < argc) data_dir = argv[++index];
    else if(argument == "--screenshot" && index + 1 < argc) screenshot = argv[++index];
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
  earthsiege::framebuffer frame;
  std::unique_ptr<earthsiege::window> display;
  if(!headless) display = std::make_unique<earthsiege::window>(scale);
  else if(!frame_limit) frame_limit = 1;
  bool running{true};
  unsigned int frames{};
  while(running) {
    SDL_Event event{};
    while(display && display->poll(event)) {
      if(event.type == SDL_QUIT) running = false;
      if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
      if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) menu.cancel_press();
      if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) menu.mouse_down(event.button.x, event.button.y);
      if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        auto const action{menu.mouse_up(event.button.x, event.button.y)};
        if(action == 1003) running = false;
        else if(action) std::cout << "Menu action " << *action << std::endl;
      }
    }
    menu.draw(frame);
    if(display) display->present(frame, menu.current_palette());
    ++frames;
    if(frame_limit && frames >= frame_limit) running = false;
    if(display) SDL_Delay(16);
  }
  if(!screenshot.empty()) frame.save(screenshot, menu.current_palette());
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
