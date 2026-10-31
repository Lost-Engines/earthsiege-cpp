#include "window.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include "resources/artwork.h"
#include "render/software_cursor.h"

namespace earthsiege {

sdl_context::sdl_context() {
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) throw std::runtime_error{SDL_GetError()};
}
sdl_context::~sdl_context() { SDL_Quit(); }

window::window(unsigned int scale) {
  /// Fit an integer-scaled floating window inside the desktop's usable area.
  if(!scale) {
    SDL_Rect usable{};
    if(SDL_GetDisplayUsableBounds(0, &usable) == 0) {
      int constexpr decoration_margin{64};
      scale = static_cast<unsigned int>(std::max(1, std::min((usable.w - decoration_margin) / static_cast<int>(screen_width), (usable.h - decoration_margin) / static_cast<int>(screen_height))));
    } else scale = 2;
  }
  scale = std::clamp(scale, 1u, 16u);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_ShowCursor(SDL_DISABLE);
  handle.reset(SDL_CreateWindow("Metaltech: Earthsiege C++ - Ctrl+Shift+M: music hardware", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, static_cast<int>(screen_width * scale), static_cast<int>(screen_height * scale), SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
  if(!handle) throw std::runtime_error{SDL_GetError()};
  renderer.reset(SDL_CreateRenderer(handle.get(), -1, SDL_RENDERER_SOFTWARE));
  if(!renderer) throw std::runtime_error{SDL_GetError()};
  SDL_RenderSetLogicalSize(renderer.get(), screen_width, screen_height);
  SDL_RenderSetIntegerScale(renderer.get(), SDL_TRUE);
  texture.reset(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height));
  if(!texture) throw std::runtime_error{SDL_GetError()};
}

void window::present(framebuffer const &frame,palette const &colours,software_cursor const *cursor) {
  auto presented{frame};
  if(cursor&&SDL_GetMouseFocus()==handle.get()){
    auto const p{pointer_position()};
    if(p[0]>=0&&p[0]<320&&p[1]>=0&&p[1]<200)cursor->draw(presented,p[0],p[1]);
  }
  auto const &pixels{presented.pixels()};
  for(unsigned int index{}; index < pixels.size(); ++index) display[index] = colours.colours[pixels[index]];
  if(SDL_UpdateTexture(texture.get(), nullptr, display.data(), screen_width * sizeof(uint32_t)) != 0) throw std::runtime_error{SDL_GetError()};
  SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(renderer.get());
  SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
  SDL_RenderPresent(renderer.get());
}

std::array<int,2> window::pointer_position()const{
  int wx{},wy{},x{},y{};SDL_GetMouseState(&wx,&wy);mouse_position(wx,wy,x,y);return {x,y};
}

auto window::poll(SDL_Event &event)->bool { return SDL_PollEvent(&event) != 0; }

void window::warp_mouse(int const x,int const y) {
  int window_x{},window_y{};SDL_RenderLogicalToWindow(renderer.get(),static_cast<float>(x),static_cast<float>(y),&window_x,&window_y);
  SDL_WarpMouseInWindow(handle.get(),window_x,window_y);
}

void window::mouse_position(int window_x, int window_y, int &x, int &y) const {
  float logical_x{};
  float logical_y{};
  SDL_RenderWindowToLogical(renderer.get(), window_x, window_y, &logical_x, &logical_y);
  x = static_cast<int>(logical_x);
  y = static_cast<int>(logical_y);
}

} // namespace earthsiege
