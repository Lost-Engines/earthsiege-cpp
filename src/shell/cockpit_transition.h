#pragma once
#include <compare>
#include <cstdint>
#include <array>
#include "render/framebuffer.h"

namespace earthsiege {
/// Original registered edge rectangles, in logical window coordinates.
std::array<rectangle,3> cockpit_edge_bounds(int view);
int cockpit_edge_request(int view,size_t edge);
int cockpit_edge_cursor(int view,int x,int y);
/// Presentation ownership recovered from DBSIM 22634/226f0/22904/59b40.
/// The VGA draw-page pair also determines when a held view request can repeat.
class cockpit_transition {
public:
  struct state {
    int32_t view{},pending{-1},prepared{},cooldown{},saved_view{},draw_page{2},other_page{};
    auto operator<=>(state const &)const=default;
  };
private:
  state value;
public:
  cockpit_transition()=default;
  explicit cockpit_transition(state initial):value{initial}{}
  state const &snapshot()const{return value;}
  void request(int action);
  void present();
  /// Host inspection shortcuts bypass the original transition animation.
  void reset(int view);
};
} // namespace earthsiege
