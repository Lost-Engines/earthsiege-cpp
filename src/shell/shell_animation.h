#pragma once
#include "resources/presentation.h"
#include "simulation/random_state.h"
namespace earthsiege {
struct shell_animation_state {
 uint16_t page{};
 uint32_t mode{};
 bool wait_latched{},fade_latched{},executed{};
 int16_t remaining{},period{},jump{-1},branch{-1},depth{},x{},y{};
};
struct shell_animation_command {
 uint16_t opcode{};
 std::vector<int32_t> arguments;
 std::string resource;
};
/// GO's TTM scheduler is distinct from the DBSIM cockpit player. Commands
/// preserve painter order; FIX_RECT commits to the resident background.
class shell_animation {
 presentation_program program;
 shell_animation_state current;
 bool repeat{};
 uint8_t shape_bank{};
 std::vector<shell_animation_command> output;
 void execute(uint16_t page,random_state &random,unsigned recursion=0);
 void next_page();
 void restart();
public:
 explicit shell_animation(presentation_program program,bool repeat=false);
 shell_animation_state const &state()const{return current;}
 std::vector<shell_animation_command> const &commands()const{return output;}
 void select_page(uint16_t page);
 void advance(int16_t elapsed,random_state &random);
};
}
