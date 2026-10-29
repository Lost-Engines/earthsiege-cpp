#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <SDL.h>
#include "platform/window.h"
#include "platform/audio_output.h"
#include "platform/joystick_devices.h"
#include "resources/resource_store.h"
#include "shell/game_shell.h"
#include "shell/mission_alert.h"
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

auto command(SDL_Keycode const code, bool const alt, bool const control)->std::optional<uint16_t> {
  uint16_t scan;
  switch(code) {
    case SDLK_ESCAPE: scan=1; break;
    case SDLK_INSERT: scan=82; break;
    case SDLK_F9: scan=67; break;
    case SDLK_F10: scan=68; break;
    case SDLK_SPACE: scan=57; break;
    case SDLK_p: scan=25; break;
    case SDLK_q: scan=16; break;
    case SDLK_F12: scan=88; break;
    case SDLK_d: scan=32; break;
    case SDLK_t: scan=20; break;
    case SDLK_x: scan=45; break;
    case SDLK_s: scan=31; break;
    case SDLK_i: scan=23; break;
    case SDLK_COMMA: scan=51; break;
    case SDLK_PERIOD: scan=52; break;
    case SDLK_RETURN: scan=28; break;
    case SDLK_KP_5: scan=76; break;
    case SDLK_KP_8: case SDLK_UP: scan=72; break;
    case SDLK_KP_2: case SDLK_DOWN: scan=80; break;
    case SDLK_KP_4: case SDLK_LEFT: scan=75; break;
    case SDLK_KP_6: case SDLK_RIGHT: scan=77; break;
    case SDLK_PAGEUP: scan=73; break;
    case SDLK_PAGEDOWN: scan=81; break;
    case SDLK_a: scan=30; break;
    case SDLK_g: scan=34; break;
    case SDLK_h: scan=35; break;
    case SDLK_o: scan=24; break;
    case SDLK_c: scan=46; break;
    case SDLK_e: scan=18; break;
    case SDLK_f: scan=33; break;
    case SDLK_w: scan=17; break;
    case SDLK_b: scan=48; break;
    case SDLK_z: scan=44; break;
    case SDLK_r: scan=19; break;
    case SDLK_TAB: scan=15; break;
    case SDLK_BACKSPACE: scan=14; break;
    case SDLK_MINUS: scan=12; break;
    case SDLK_EQUALS: scan=13; break;
    case SDLK_KP_MINUS: scan=74; break;
    case SDLK_KP_PLUS: scan=78; break;
    default:
      if(code>=SDLK_F1 && code<=SDLK_F8) scan=static_cast<uint16_t>(59+code-SDLK_F1);
      else if(code>=SDLK_1 && code<=SDLK_9) scan=static_cast<uint16_t>(2+code-SDLK_1);
      else return std::nullopt;
  }
  return static_cast<uint16_t>(scan | (alt ? 0x200 : 0) | (control ? 0x400 : 0));
}

} // namespace

auto main(int argc, char *argv[])->int try {
  std::filesystem::path data_dir{earthsiege::resource_store::locate(std::filesystem::absolute(argv[0]))};
  std::filesystem::path screenshot,save_directory;
  bool list{};
  bool headless{};
  bool skip_intro{};
  unsigned int scale{};
  std::optional<unsigned int> inspect_demo;
  unsigned int frame_limit{};
  for(int index{1}; index < argc; ++index) {
    std::string_view const argument{argv[index]};
    if(argument == "--help") {
      std::cout << "Earthsiege C++\nUsage: earthsiege [options]\n"
        "  --data-dir PATH       Original game data (default: bundled assets)\n"
        "  --save-dir PATH       Override the separate user save directory\n"
        "  --inspect-demo N     Inspect demo 1–4 terrain and player roster (not replay)\n"
        "  --scale N             Integer window scale (default: largest fit)\n"
        "  --screenshot PATH     Save an unscaled PPM frame\n"
        "  --frames N            Exit after N frames\n"
        "  --skip-intro          Skip opening and pilot-death cinematics (original -a)\n"
        "  --headless            Render a frame without creating a window\n"
        "  --list-resources      Print all original resource sizes and CRCs\n";
      return EXIT_SUCCESS;
    }
    if(argument == "--data-dir" && index + 1 < argc) data_dir = argv[++index];
    else if(argument == "--save-dir" && index + 1 < argc) save_directory = argv[++index];
    else if(argument == "--screenshot" && index + 1 < argc) screenshot = argv[++index];
    else if(argument == "--inspect-demo" && index + 1 < argc) inspect_demo = integer_option(argv[++index]);
    else if(argument == "--scale" && index + 1 < argc) scale = integer_option(argv[++index]);
    else if(argument == "--frames" && index + 1 < argc) frame_limit = integer_option(argv[++index]);
    else if(argument == "--skip-intro") skip_intro=true;
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
  if(save_directory.empty()) {
    char *path{SDL_GetPrefPath("lost-engines","earthsiege")};
    if(!path)throw std::runtime_error{SDL_GetError()};
    save_directory=path;SDL_free(path);
  }
  earthsiege::game_shell menu{resources,save_directory,static_cast<int>(std::time(nullptr)&127)};
  if(!inspect_demo)menu.startup(!skip_intro);
  std::unique_ptr<earthsiege::mission_alert> alert;
  earthsiege::framebuffer alert_frame;
  earthsiege::framebuffer menu_frame;
  std::unique_ptr<earthsiege::mission_view> scene;
  if(inspect_demo) {
    scene = std::make_unique<earthsiege::mission_view>(resources, *inspect_demo);
    scene->configure_input(menu.settings(),menu.device_calibrations());
    std::cout << "Scene inspection: arrows/keypad walk and turn; I/J/K/M aim; Alt increases aiming speed.\n"
      "Ctrl + arrows move camera; Ctrl + Shift + up/down change distance.\n"
      "Ctrl+F1 cockpit, Ctrl+F2 orbit, Ctrl+F3 free camera; P pauses, Escape returns to menu.\n"
      "F7 tactical map, F8 damage, F9 left, F10 right cockpit view.\n"
      "Space fires; 1–9 or W/Alt+W selects weapons; Tab selects targets; Enter confirms tactical orders; R toggles radar.\n"
      "Hold right mouse to aim; both buttons centre aim. Backspace recentres.\n"
      "B changes weapon groups; Alt+1–9 edits a group; +/- adjusts charge; Z restores charge settings.\n"
      "This inspection view includes mission AI and combat; the cockpit is still being integrated.\n";
  }
  std::unique_ptr<earthsiege::window> display;
  if(!headless){display = std::make_unique<earthsiege::window>(scale);SDL_StartTextInput();}
  else if(!frame_limit) frame_limit = 1;
  std::unique_ptr<earthsiege::audio_output> sound;
  if(display)try{sound=std::make_unique<earthsiege::audio_output>();}catch(std::exception const &e){std::cerr<<"Audio unavailable: "<<e.what()<<'\n';}
  std::unique_ptr<earthsiege::joystick_devices> sticks;
  if(display)try{sticks=std::make_unique<earthsiege::joystick_devices>();}catch(std::exception const &e){std::cerr<<"Joystick input unavailable: "<<e.what()<<'\n';}
  bool running{true};
  bool focused{true};
  unsigned int frames{};
  uint64_t previous{display ? SDL_GetTicks64() : 0};
  uint64_t pending{0};
  auto const open_alert{[&](int message){
    alert=std::make_unique<earthsiege::mission_alert>(resources,message);
    if(scene)scene->clear_input();
    if(sound)sound->pause(true);
    pending=0;
  }};
  auto const respond{[&](int response){
    int const message{alert->message()};bool const ending{alert->exits_mission(response)};alert.reset();
    if(sound){sound->clear();sound->pause(!focused);}
    if(message==1 && response==1){running=false;return;}
    if(scene && message>1 && ending){
      if(!inspect_demo)menu.mission_return(scene->results(),scene->mission_variables(),static_cast<int16_t>(std::time(nullptr)&127));
      scene.reset();pending=0;
    }
  }};
  while(running) {
    SDL_Event event{};
    while(display && display->poll(event)) {
      if(event.type == SDL_QUIT) running = false;
      if(event.type == SDL_WINDOWEVENT) {
        if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          focused = false;
          if(sound)sound->pause(true);
          menu.cancel_press();
          if(scene) scene->clear_input();
        }
        if(event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED){focused = true;if(sound)sound->pause(alert!=nullptr);}
      }
      if(alert) {
        if(focused && event.type==SDL_KEYDOWN && !event.key.repeat) {
          if(auto scan{command(event.key.keysym.sym,false,false)})if(auto response{alert->key(static_cast<uint8_t>(*scan))})respond(*response);
        } else if(focused && event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT)alert->mouse_down(event.button.x,event.button.y);
        else if(focused && event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT)if(auto response{alert->mouse_up(event.button.x,event.button.y)})respond(*response);
        continue;
      }
      if(event.type==SDL_TEXTINPUT && !scene){menu.text_input(event.text.text);continue;}
      if(event.type == SDL_KEYDOWN && !event.key.repeat) {
        auto const key{event.key.keysym.sym};
        if(!scene){if(menu.presentation_active())menu.key(1);else if(auto scan{command(key,(event.key.keysym.mod&KMOD_ALT)!=0,(event.key.keysym.mod&KMOD_CTRL)!=0)})menu.key(*scan);}
        else if(focused) {
          if(key==SDLK_ESCAPE && inspect_demo){scene.reset();if(sound)sound->clear();continue;}
          if((key==SDLK_ESCAPE || key==SDLK_q) && (event.key.keysym.mod&KMOD_CTRL)){open_alert(1);continue;}
          if(key==SDLK_p && !(event.key.keysym.mod&KMOD_ALT)){open_alert(0);continue;}
          if(key==SDLK_q){int const status{static_cast<int>(scene->status(true))};open_alert(status<0?5:status);continue;}
          if(key==SDLK_F1 && (event.key.keysym.mod&KMOD_CTRL))scene->change_cockpit_view(earthsiege::cockpit_view::forward);
          if(key==SDLK_F2 && (event.key.keysym.mod&KMOD_CTRL))scene->change_camera(earthsiege::camera_mode::orbit);
          if(key==SDLK_F3 && (event.key.keysym.mod&KMOD_CTRL))scene->change_camera(earthsiege::camera_mode::free);
        }
      }
      if(scene) {
        if(focused && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && !event.key.repeat) {
          bool const alt{(event.key.keysym.mod & KMOD_ALT)!=0};
          if(auto const key{held_key(event.key.keysym.sym)}) scene->key(*key, event.type == SDL_KEYDOWN, alt);
          if(event.type==SDL_KEYDOWN)if(auto const code{command(event.key.keysym.sym,alt,(event.key.keysym.mod & KMOD_CTRL)!=0)})scene->command(*code);
        }
        if(focused && event.type==SDL_MOUSEMOTION) scene->mouse_position(event.motion.x,event.motion.y);
        if(focused && (event.type==SDL_MOUSEBUTTONDOWN || event.type==SDL_MOUSEBUTTONUP)) {
          scene->mouse_position(event.button.x,event.button.y);
          if(event.button.button==SDL_BUTTON_LEFT || event.button.button==SDL_BUTTON_RIGHT)scene->mouse_button(event.button.button==SDL_BUTTON_RIGHT,event.type==SDL_MOUSEBUTTONDOWN);
        }
      } else if(focused) {
        if(event.type == SDL_MOUSEMOTION) menu.mouse_move(event.motion.x,event.motion.y);
        if(event.type == SDL_MOUSEBUTTONDOWN && (event.button.button == SDL_BUTTON_LEFT||event.button.button == SDL_BUTTON_RIGHT)) menu.mouse_down(event.button.x,event.button.y,event.button.button==SDL_BUTTON_RIGHT);
        if(event.type == SDL_MOUSEBUTTONUP && (event.button.button == SDL_BUTTON_LEFT||event.button.button == SDL_BUTTON_RIGHT)) menu.mouse_up(event.button.x,event.button.y,event.button.button==SDL_BUTTON_RIGHT);
      }
    }
    if(focused&&!alert&&sticks){
      auto const &p{menu.settings().current()};auto raw{sticks->poll(p[7],p[9]!=0)};
      if(scene)scene->devices(raw,sticks->throttle());else menu.devices(raw);
    }
    if(auto const warp{menu.take_pointer_warp()};warp&&display)display->warp_mouse(warp->x,warp->y);
    if(menu.quit_requested())running=false;
    if(menu.ready_to_launch()) {
      inspect_demo.reset();scene=std::make_unique<earthsiege::mission_view>(resources,menu.launch());scene->configure_input(menu.settings(),menu.device_calibrations());scene->environment_detail(menu.settings().current()[3]!=0,menu.settings().current()[4],menu.settings().current()[5]);pending=0;
      if(sound)sound->clear();
    }
    auto const now{display ? SDL_GetTicks64() : previous + 25};
    pending += focused && !alert ? std::min<uint64_t>(now - previous, 100) : 0;
    previous = now;
    if(!focused || alert) pending = 0;
    if(scene) {
      if(frames) while(pending >= 25) {
        auto const modifiers{display ? SDL_GetModState() : KMOD_NONE};
        scene->step((modifiers & KMOD_CTRL) != 0, (modifiers & KMOD_SHIFT) != 0);
        if(auto const warp{scene->take_pointer_warp()};warp && display)display->warp_mouse(warp->x,warp->y);
        auto const samples{scene->take_audio()};if(sound)sound->queue(samples);
        pending -= 25;
        if(auto message{scene->take_mission_prompt()}){open_alert(static_cast<int>(*message));break;}
      }
      else pending = 0;
      scene->draw();
    } else {
      while(pending>=25){menu.advance(3);auto samples{menu.take_audio()};if(sound)sound->queue(samples);pending-=25;}
      menu.draw(menu_frame);
    }
    if(alert){alert_frame=scene?scene->image():menu_frame;alert->draw(alert_frame);}
    auto const &frame{alert?alert_frame:scene ? scene->image() : menu_frame};
    auto const &colours{scene ? scene->colours() : menu.colours()};
    if(display) display->present(frame, colours);
    ++frames;
    if(frame_limit && frames >= frame_limit) running = false;
    if(display) SDL_Delay(8);
  }
  if(!screenshot.empty()) {
    (alert ? alert_frame : scene ? scene->image() : menu_frame).save(screenshot, scene ? scene->colours() : menu.colours());
  }
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
