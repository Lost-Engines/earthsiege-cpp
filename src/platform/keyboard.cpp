#include "keyboard.h"
#include <GLFW/glfw3.h>

namespace earthsiege {
auto keyboard_held_key(int const code)->std::optional<earthsiege::held_key> {
  using enum earthsiege::held_key;
  switch(code) {
    case GLFW_KEY_KP_7: case GLFW_KEY_HOME: return forward_left;
    case GLFW_KEY_KP_8: case GLFW_KEY_UP: return forward;
    case GLFW_KEY_KP_9: case GLFW_KEY_PAGE_UP: return forward_right;
    case GLFW_KEY_KP_4: case GLFW_KEY_LEFT: return left;
    case GLFW_KEY_KP_6: case GLFW_KEY_RIGHT: return right;
    case GLFW_KEY_KP_1: case GLFW_KEY_END: return reverse_left;
    case GLFW_KEY_KP_2: case GLFW_KEY_DOWN: return reverse;
    case GLFW_KEY_KP_3: case GLFW_KEY_PAGE_DOWN: return reverse_right;
    case GLFW_KEY_M: return aim_down;
    case GLFW_KEY_J: return aim_left;
    case GLFW_KEY_K: return aim_right;
    case GLFW_KEY_I: return aim_up;
    case GLFW_KEY_SPACE: return fire;
    case GLFW_KEY_KP_ENTER: case GLFW_KEY_ENTER: return enter;
    default: return std::nullopt;
  }
}

auto keyboard_command(int const code, bool const alt, bool const control)->std::optional<uint16_t> {
  uint16_t scan;
  switch(code) {
    case GLFW_KEY_ESCAPE: scan=1; break;
    case GLFW_KEY_INSERT: case GLFW_KEY_KP_0: scan=82; break;
    case GLFW_KEY_F9: scan=67; break;
    case GLFW_KEY_F10: scan=68; break;
    case GLFW_KEY_SPACE: scan=57; break;
    case GLFW_KEY_P: scan=25; break;
    case GLFW_KEY_Q: scan=16; break;
    case GLFW_KEY_F12: scan=88; break;
    case GLFW_KEY_F11: scan=87; break;
    case GLFW_KEY_N: scan=49; break;
    case GLFW_KEY_HOME: case GLFW_KEY_KP_7: scan=71; break;
    case GLFW_KEY_END: case GLFW_KEY_KP_1: scan=79; break;
    case GLFW_KEY_D: scan=32; break;
    case GLFW_KEY_T: scan=20; break;
    case GLFW_KEY_X: scan=45; break;
    case GLFW_KEY_S: scan=31; break;
    case GLFW_KEY_I: scan=23; break;
    case GLFW_KEY_COMMA: scan=51; break;
    case GLFW_KEY_PERIOD: scan=52; break;
    case GLFW_KEY_KP_ENTER: case GLFW_KEY_ENTER: scan=28; break;
    case GLFW_KEY_KP_5: scan=76; break;
    case GLFW_KEY_KP_8: case GLFW_KEY_UP: scan=72; break;
    case GLFW_KEY_KP_2: case GLFW_KEY_DOWN: scan=80; break;
    case GLFW_KEY_KP_4: case GLFW_KEY_LEFT: scan=75; break;
    case GLFW_KEY_KP_6: case GLFW_KEY_RIGHT: scan=77; break;
    case GLFW_KEY_PAGE_UP: case GLFW_KEY_KP_9: scan=73; break;
    case GLFW_KEY_PAGE_DOWN: case GLFW_KEY_KP_3: scan=81; break;
    case GLFW_KEY_A: scan=30; break;
    case GLFW_KEY_G: scan=34; break;
    case GLFW_KEY_H: scan=35; break;
    case GLFW_KEY_O: scan=24; break;
    case GLFW_KEY_C: scan=46; break;
    case GLFW_KEY_E: scan=18; break;
    case GLFW_KEY_F: scan=33; break;
    case GLFW_KEY_W: scan=17; break;
    case GLFW_KEY_L: scan=38; break;
    case GLFW_KEY_GRAVE_ACCENT: scan=41; break;
    case GLFW_KEY_LEFT_BRACKET: scan=26; break;
    case GLFW_KEY_RIGHT_BRACKET: scan=27; break;
    case GLFW_KEY_V: scan=47; break;
    case GLFW_KEY_B: scan=48; break;
    case GLFW_KEY_Z: scan=44; break;
    case GLFW_KEY_R: scan=19; break;
    case GLFW_KEY_TAB: scan=15; break;
    case GLFW_KEY_BACKSPACE: scan=14; break;
    case GLFW_KEY_MINUS: scan=12; break;
    case GLFW_KEY_EQUAL: scan=13; break;
    case GLFW_KEY_KP_SUBTRACT: scan=74; break;
    case GLFW_KEY_KP_ADD: scan=78; break;
    default:
      if(code>=GLFW_KEY_F1 && code<=GLFW_KEY_F8) scan=static_cast<uint16_t>(59+code-GLFW_KEY_F1);
      else if(code>=GLFW_KEY_1 && code<=GLFW_KEY_9) scan=static_cast<uint16_t>(2+code-GLFW_KEY_1);
      else return std::nullopt;
  }
  return static_cast<uint16_t>(scan | (alt ? 0x200 : 0) | (control ? 0x400 : 0));
}

} // namespace earthsiege
