#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <SDL.h>
#include "render/framebuffer.h"

namespace earthsiege {

struct palette;
class software_cursor;

struct sdl_context {
  sdl_context();
  ~sdl_context();
  sdl_context(sdl_context const &) = delete;
  auto operator=(sdl_context const &)->sdl_context & = delete;
};

class window {
  sdl_context context;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> handle{nullptr, SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{nullptr, SDL_DestroyRenderer};
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{nullptr, SDL_DestroyTexture};
  std::array<uint32_t, screen_width * screen_height> display{};

public:
  explicit window(unsigned int scale = 0);
  void present(framebuffer const &frame, palette const &colours,software_cursor const *cursor=nullptr);
  std::array<int,2> pointer_position()const;
  auto poll(SDL_Event &event)->bool;
  void warp_mouse(int x,int y);
  void mouse_position(int window_x, int window_y, int &x, int &y) const;
};

} // namespace earthsiege
