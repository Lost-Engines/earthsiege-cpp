#pragma once
#include "render/tactical_map.h"
#include "resources/mission_text.h"
namespace earthsiege {
struct briefing_map_geometry {
  std::vector<vec3<int32_t>> points;
  std::vector<vec3<int32_t>> route;
  std::vector<base_spawn> bases;
  std::array<vec3<int32_t>, 8> players{};
  std::array<int16_t, 8> members{};
  std::array<bool, 8> position_defined{};
  vec3<int32_t> full_view{};
  vec3<int32_t> squad_view{};
  vec3<int32_t> minimum{};
  vec3<int32_t> maximum{};
  int player_count{0};
  bool hide_enemy{false};
};
briefing_map_geometry make_briefing_map(resource_store const&, mission_script const&, int player_count);
struct briefing_map_state {
  vec3<int32_t> camera{};
  vec2<int32_t> pan{};
  int players{0};
  int route_points{0};
  int waypoints{0};
  int stage{19};
  int objective{0};
  int resume{0};
  uint32_t deadline{0};
  bool ready{true};
};
/// GO's overhead briefing map, separate from the cockpit tactical display.
class briefing_map {
 model_resource terrain;
 std::vector<scenery_object> scenery;
 palette colours;
 std::vector<bitmap> textures;
 std::vector<bitmap> icons;
 font text_face;
 std::vector<std::string> objectives;
 struct map_label {
   vec3<int32_t> position;
   std::string text;
   int size;
 };
 std::vector<map_label> labels;
 int selected_label{-1};
 briefing_map_geometry data_;
 briefing_map_state state_;
 briefing_map_state display_state_;
 vec3<int32_t> from{};
 vec3<int32_t> destination{};
 vec3<int32_t> increment{};
 int segment_ticks{0};
 int route_index{1};
 int remaining_pages{2};
 bool text_latch{false};
 bool revealing{false};
 bool reveal_clock_started{false};
 uint32_t reveal_start{0};
 uint32_t reveal_elapsed{0};
 uint32_t reveal_duration{0};
 std::string last_objective;
 int32_t pan_step{20000};
public:
 briefing_map(resource_store const&, mission_script const&, int player_count, std::vector<std::string> objectives = {});
 briefing_map_geometry const &geometry() const {
   return data_;
 }
 briefing_map_state const &state() const {
   return state_;
 }
 // Opening uses the two ten-tick pauses and one tick per grid line.
 void start_tour(bool opening = false);
 void skip_tour();
 void update(uint32_t timer);
 void navigate(uint16_t action);
 void restore_view();
 void hover(int x, int y, rectangle bounds = {15, 12, 283, 143});
 void draw(framebuffer&, rectangle bounds = {15, 12, 283, 143});
};
}
