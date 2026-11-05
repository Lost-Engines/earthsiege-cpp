#pragma once

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>
#include "render/framebuffer.h"

namespace earthsiege {

struct palette;
class software_cursor;

enum class event_type { close, focus, key_down, key_up, text, motion, button_down, button_up };
struct window_event {
  event_type type{};
  int key{};
  int modifiers{};
  int button{};
  int x{};
  int y{};
  bool repeat{};
  bool focused{};
  std::string text{};
};

struct glfw_context {
  glfw_context();
  ~glfw_context();
  glfw_context(glfw_context const &) = delete;
  auto operator=(glfw_context const &)->glfw_context & = delete;
};

class window {
  glfw_context context;
  std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> handle{nullptr, glfwDestroyWindow};
  unsigned int texture{};
  std::array<std::array<uint8_t, 4>, screen_width * screen_height> display{};
  std::deque<window_event> events;
  static auto self(GLFWwindow *handle)->window &;
  auto logical_position(double x, double y) const->std::array<int, 2>;

public:
  explicit window(unsigned int scale = 0);
  ~window();
  window(window const &) = delete;
  auto operator=(window const &)->window & = delete;
  void draw(framebuffer const &frame, palette const &colours, software_cursor const *cursor = nullptr);
  void present(framebuffer const &frame, palette const &colours, software_cursor const *cursor = nullptr);
  auto pointer_position() const->std::array<int, 2>;
  auto poll(window_event &event)->bool;
  void pump_events();
  void warp_mouse(int x, int y);
  auto modifiers() const->int;
  auto focused() const->bool;
  auto get() const->GLFWwindow *;
};

} // namespace earthsiege
