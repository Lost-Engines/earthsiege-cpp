#pragma once
#include "render/tactical_map.h"
#include "resources/mission_text.h"
namespace earthsiege {
struct briefing_map_geometry {
 std::vector<vec3<int32_t>> points,route;
 std::vector<base_spawn> bases;
 std::array<vec3<int32_t>,8> players{};
 std::array<int16_t,8> members{};
 std::array<bool,8> position_defined{};
 vec3<int32_t> full_view{},squad_view{},minimum{},maximum{};
 int player_count{};
 bool hide_enemy{};
};
briefing_map_geometry make_briefing_map(resource_store const &,mission_script const &,int player_count);
struct briefing_map_state {
 vec3<int32_t> camera{};
 vec2<int32_t> pan{};
 int players{},route_points{},waypoints{};
 int stage{19},objective{},resume{};
 uint32_t deadline{};
 bool ready{true};
};
/// GO's overhead briefing map, separate from the cockpit tactical display.
class briefing_map {
 model_resource terrain;
 std::vector<scenery_object> scenery;
 palette colours;
 std::vector<bitmap> textures,icons;
 font text_face;
 std::vector<std::string> objectives;
 struct map_label {vec3<int32_t> position;std::string text;int size;};
 std::vector<map_label> labels;
 int selected_label{-1};
 briefing_map_geometry data_;
 briefing_map_state state_,display_state_;
 vec3<int32_t> from{},destination{},increment{};
 int segment_ticks{},route_index{1},remaining_pages{2};
 bool text_latch{};
 bool revealing{},reveal_clock_started{};
 uint32_t reveal_start{},reveal_elapsed{},reveal_duration{};
 std::string last_objective;
 int32_t pan_step{20000};
public:
 briefing_map(resource_store const &,mission_script const &,int player_count,std::vector<std::string> objectives={});
 briefing_map_geometry const &geometry()const{return data_;}
 briefing_map_state const &state()const{return state_;}
 // Opening uses the two ten-tick pauses and one tick per grid line.
 void start_tour(bool opening=false);
 void skip_tour();
 void update(uint32_t timer);
 void navigate(uint16_t action);
 void restore_view();
 void hover(int x,int y,rectangle bounds={15,12,283,143});
 void draw(framebuffer &,rectangle bounds={15,12,283,143});
};
}
