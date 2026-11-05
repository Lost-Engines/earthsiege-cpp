#include "platform/window.h"
#include "resources/artwork.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, char const *message) {
  if(!value) throw std::runtime_error{message};
}
}
int main() try {
  earthsiege::window display{2};
  auto *handle{display.get()};
  earthsiege::window_event event;
  while(display.poll(event)) {}
  // Dispatch through the registered GLFW callbacks, then consume the game queue.
  auto const key{glfwSetKeyCallback(handle, nullptr)};
  glfwSetKeyCallback(handle, key);
  key(handle, GLFW_KEY_P, 0, GLFW_PRESS, 0);
  key(handle, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
  require(display.poll(event) && event.key == GLFW_KEY_P, "pause stays ahead of dismissal");
  require(display.poll(event) && event.key == GLFW_KEY_ESCAPE, "dismissal remains queued for modal");
  key(handle, GLFW_KEY_F1, 0, GLFW_REPEAT, GLFW_MOD_CONTROL);
  require(display.poll(event) && event.repeat && event.modifiers == GLFW_MOD_CONTROL, "repeat and modifiers");
  auto const character{glfwSetCharCallback(handle, nullptr)};
  glfwSetCharCallback(handle, character);
  character(handle, 'R');
  require(display.poll(event) && event.text == "R", "text entry callback");
  character(handle, 0xe9);
  require(display.poll(event) && event.text == "\xc3\xa9", "UTF-8 text entry");
  auto const focus{glfwSetWindowFocusCallback(handle, nullptr)};
  glfwSetWindowFocusCallback(handle, focus);
  focus(handle, GLFW_FALSE);
  require(display.poll(event) && event.type == earthsiege::event_type::focus && !event.focused, "focus loss notification");
  focus(handle, GLFW_TRUE);
  require(display.poll(event) && event.focused, "focus restoration");
  auto const motion{glfwSetCursorPosCallback(handle, nullptr)};
  glfwSetCursorPosCallback(handle, motion);
  motion(handle, 20, 40);
  motion(handle, 40, 80);
  require(display.poll(event) && event.x == 10 && event.y == 20, "first queued cursor position");
  require(display.poll(event) && event.x == 20 && event.y == 40, "second queued cursor position");
  earthsiege::framebuffer frame;
  earthsiege::palette colours;
  colours.colours[0] = 0xff123456;
  colours.colours[1] = 0xffedcba9;
  frame.fill({0, 0, 159, 99}, 1);
  display.draw(frame, colours);
  // Read the rendered back buffer before the compositor takes ownership.
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  std::array<uint8_t, 3> pixel{};
  glReadPixels(100, 300, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
  require(pixel == std::array<uint8_t, 3>{0xed, 0xcb, 0xa9}, "top-left presentation orientation and RGB channels");
  glReadPixels(500, 100, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
  require(pixel == std::array<uint8_t, 3>{0x12, 0x34, 0x56}, "bottom-right presentation pixels");
  glfwSetWindowShouldClose(handle, GLFW_TRUE);
  while(display.poll(event) && event.type != earthsiege::event_type::close) {}
  require(event.type == earthsiege::event_type::close, "close survives nested dialog consumption");
  std::cout << "GLFW events and OpenGL framebuffer checks pass\n";
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
