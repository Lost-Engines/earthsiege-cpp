#pragma once
#include "shell/mfd_controller.h"
#include "simulation/instrument_failure.h"
#include "shell/pointer_controls.h"
#include "render/damage_display.h"
#include "render/scanner_display.h"
#include "render/navigation_display.h"
#include "render/missile_display.h"
#include "simulation/mission_session.h"

namespace earthsiege {
/// Connect the six original pages to mission state and virtual cockpit controls.
class cockpit_displays {
  random_state &random;
  instrument_failure_state failure;
  font failure_font;
  std::string failure_label;
  std::vector<uint16_t> sounds;
  mfd_instruments controls;
  mfd_controller controller;
  communications_display communications;
  communications_display_state choices;
  damage_display status, target;
  scanner_display scanner;
  framebuffer cache;
  navigation_display navigation;
  missile_display missile;
  missile_camera_tracker tracker;
  uint64_t watched_sequence{};
  std::array<size_t,mfd_button_count> buttons;
  std::array<size_t,6> rows;
  rectangle painted_bounds{};
  bool visible{}, initialized{};
  void enable_controls(pointer_controls &input);
  void transmit(size_t choice, mission_session &session);
  void paint(mfd_page page, mission_session &session, camera_state const &camera, uint32_t ticks);
public:
  cockpit_displays(resource_store const &resources, cockpit_layout const &layout, mission_session &session, pointer_controls &input, random_state &ui_random);
  std::vector<uint16_t> take_sounds();
  bool command(uint16_t code, mission_session &session, pointer_controls &input);
  bool activate(control_activation const &action, mission_session &session, pointer_controls &input);
  void update(mission_session &session, pointer_controls &input, camera_state const &camera, uint32_t ticks, int origin_x, int origin_y);
  void draw(framebuffer &frame, pointer_controls const &input, int offset_x, int offset_y) const;
  mfd_state const &state() const;
};
} // namespace earthsiege
