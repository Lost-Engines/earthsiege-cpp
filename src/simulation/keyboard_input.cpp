#include "keyboard_input.h"
#include <utility>

namespace earthsiege {
namespace {

std::array<vec2<int32_t>, 12> constexpr directions{{
  {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1},
  {0, -1}, {-1, 0}, {1, 0}, {0, 1},
}};

} // anonymous namespace

keyboard_input::keyboard_input(keyboard_latches initial) : current{std::move(initial)} {
}

void keyboard_input::event(held_key const key, bool const pressed, bool const alt) {
  auto const index{static_cast<size_t>(key)};
  if(pressed) {
    current.held.at(index) = 1;
    current.released.at(index) = 0;
  } else if(!current.consumed.at(index)) current.released.at(index) = 1;
  else current.held.at(index) = current.released.at(index) = current.consumed.at(index) = 0;
  current.alt = alt;
}

keyboard_sample keyboard_input::sample(bool const alternate_movement) {
  keyboard_sample result;
  for(size_t index{0}; index != directions.size(); ++index) {
    if(!current.held[index]) continue;
    current.consumed[index] = 1;
    int const step{current.alt || (index < 8 && !alternate_movement) ? 256 : 64};
    auto const &direction{directions[index]};
    if(index >= 8) {
      result.aiming = vec2<int32_t>{direction.x * step, direction.y * step};
      break;                                                                 // only the first dedicated aim key contributes
    }
    bool const diagonal{direction.x != 0 && direction.y != 0};
    if(diagonal) result.movement = vec2<int32_t>{0, 0};
    result.movement.x += direction.x * step;
    result.movement.y += direction.y * step;
    if(diagonal) index = 7;                                                   // diagonals override preceding movement keys
  }
  result.fire = current.held[static_cast<size_t>(held_key::fire)] != 0;
  result.enter = current.held[static_cast<size_t>(held_key::enter)] != 0;
  for(size_t index{0}; index != held_key_count; ++index) {
    if(current.held[index] && current.released[index]) current.held[index] = current.consumed[index] = 0;
  }
  return result;
}

void keyboard_input::clear() {
  current = keyboard_latches{};
}

keyboard_latches const &keyboard_input::state() const {
  return current;
}

} // namespace earthsiege
