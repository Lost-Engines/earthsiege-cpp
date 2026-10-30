#pragma once
#include <deque>
#include "audio/mission_audio.h"
#include <memory>
#include "render/scenery_renderer.h"
#include "render/environment_renderer.h"
#include "render/instrument_bars.h"
#include "render/shield_instruments.h"
#include "render/weapon_instruments.h"
#include "render/navigation_instruments.h"
#include "render/clock_instrument.h"
#include "render/external_view.h"
#include "render/presentation_player.h"
#include "resources/cockpit.h"
#include "resources/input_tape.h"
#include "simulation/keyboard_input.h"
#include "simulation/frame_clock.h"
#include "simulation/joystick_routing.h"
#include "resources/input_options.h"
#include "shell/preferences.h"
#include "shell/pointer_controls.h"
#include "shell/cockpit_impact.h"
#include "shell/cockpit_startup.h"
#include "shell/cockpit_transition.h"
#include "render/cockpit_pan.h"
#include "render/palette_device.h"
#include "shell/cockpit_displays.h"
#include "shell/cockpit_radio.h"
#include "shell/cockpit_hdd.h"
#include "simulation/mission_session.h"

namespace earthsiege {
enum class mission_command_result { unhandled, handled, exit_mission };
struct mission_dialog_handlers {
  std::function<mission_command_result(uint16_t)> command;
  /// Return true if the selected outcome response leaves the mission.
  std::function<bool(mission_status)> outcome;
};


/// Live mission presentation and platform-independent input consumers
class mission_view {
  std::unique_ptr<mission_session> session;
  random_state &ui_random;
  mission_audio audio;
  framebuffer frame;
  cockpit_artwork cockpit;
  reactor_gauge reactor;
  throttle_gauge throttle;
  shield_instruments shields;
  weapon_instruments weapons;
  std::vector<weapon_display> painted_weapon_rows;
  navigation_instruments navigation;
  waypoint_instruments waypoints;
  clock_instrument clock;
  external_view_artwork external_artwork;
  presentation_player lamps;
  uint64_t elapsed_milliseconds{};
  frame_clock pacing;
  uint32_t interface_ticks{};
  struct weapon_control_binding {
    size_t row;
    size_t button;
    std::optional<size_t> slider;
    int32_t requested_charge;
  };
  std::vector<weapon_control_binding> weapon_controls;
  std::array<size_t, 5> weapon_groups;
  cockpit_view panel{cockpit_view::forward};
  cockpit_transition transitions;
  bool present_pending{};
  cockpit_surface surface;
  std::vector<framebuffer> pan_frames;
  cockpit_view painted_panel()const;
  palette display_colours;
  palette impact_colours;
  palette_device display_device;
  std::optional<std::array<int32_t,3>> displayed_shields;
  cockpit_impact impact;
  std::optional<cockpit_startup> startup;
  bitmap lift_artwork;
  int32_t startup_height{};
  int startup_cover{},startup_page{};
  bool startup_draw_pending{},launch_started{};
  void finish_launch();
  void update_interface_clock(uint32_t now);
  cockpit_impact_view impact_view() const;
  std::array<int32_t,2> instrument_origin() const;
  environment_renderer environment;
  earthsiege::environment_detail detail;
  scenery_renderer renderer;
  camera_state camera;
  std::optional<camera_state> options_camera;
  int options_view{};
  bool suppress_interface{};
  void update_widgets();
  void flush_interface_audio();
  keyboard_input keyboard;
  device_routing_options routing;
  std::array<joystick_calibration,2> calibrations;
  std::array<joystick_hardware,2> hardware;
  int16_t device_throttle{};
  bool mouse_enabled{true},joystick_mode_enabled{};
  void hat_view(std::array<uint8_t,4> const &);
  bool paused{false};
  bool external_camera_control{true};
  bool external_view_active{},external_view_requested{}; // 5894c applies prepared views after input.
  player_input pointer;
  pointer_controls instruments;
  std::array<size_t,3> edge_controls;
  std::unique_ptr<cockpit_displays> displays;
  std::unique_ptr<cockpit_radio> radio;
  std::unique_ptr<cockpit_hdd> hdd;
  struct pointer_event { player_input input; std::optional<uint32_t> timestamp; };
  std::deque<pointer_event> pointer_events;
  int16_t requested_throttle{};
  std::optional<bool> shield_to_front;
  void consume_pointer_events();
  void set_camera(camera_mode mode,unsigned previous_view,bool reset_input=true);
  void apply_cockpit_view(cockpit_view view,bool reset_input);
  void present_view();
  void prepare_frame(uint32_t slow_clock);
  bool step_input(player_input input,std::array<uint8_t,4> const &hat,frame_ratio ratio,bool control,mission_dialog_handlers const &dialogs={});
  std::deque<uint16_t> commands;

public:
  mission_view(resource_store const &resources, unsigned int demo);
  mission_view(resource_store const &resources, input_tape const &tape);
  mission_view(resource_store const &resources, std::unique_ptr<mission_session> mission);
  mission_results results() const { return session->results(); }
  mission_counters const &mission_variables() const { return session->mission_variables(); }
  mission_status status(bool requested=false) const { return session->mission_state(requested); }
  std::optional<mission_status> take_mission_prompt() { return session->take_mission_prompt(); }
  void apply_preferences(preferences const &);
  void configure_input(preferences const &,std::array<joystick_calibration,2> const &);
  void devices(std::array<joystick_hardware,2> const &value,int16_t lever){hardware=value;device_throttle=lever;}
  void key(held_key key, bool pressed, bool alt);
  void command(uint16_t code);
  void mouse_position(int x, int y);
  /// Original primary/secondary cursor selection; -1 means hidden.
  int pointer_cursor(int x,int y)const;
  void mouse_button(bool right, bool pressed);
  void clear_input();
  void toggle_pause();
  auto const &cloud_state()const{return environment.cloud_state();}
  camera_state const &view_camera()const{return camera;}
  void change_camera(camera_mode mode);
  void change_cockpit_view(cockpit_view view);
  void advance_time(unsigned milliseconds);
  /// Start the original nearby-friendly-base lift, or its ordinary initial draw.
  void begin_startup();
  cockpit_transition::state const &presentation_state()const{return transitions.snapshot();}
  bool starting()const{return startup.has_value();}
  /// Separate clocks retain original draw / fade / presentation ownership.
  void draw_startup(uint32_t slow_clock);
  void finish_startup_frame(uint32_t palette_clock,uint32_t end_clock);
  bool step_startup();
  bool step(bool control, bool zoom,mission_dialog_handlers const &dialogs={});
  /// Recorded axes are already device-mapped. The host supplies the separate 60 Hz presentation clock.
  void radio_input(radio_input_source source){radio->input_source(std::move(source));}
  bool step_recorded(tape_frame const &,uint32_t slow_clock,bool suppress_camera=false,mission_dialog_handlers const &dialogs={});
  mission_session const &simulation()const{return *session;}
  void begin_options();
  bool step_options();
  void step_options_recorded();
  void end_options();
  void draw();
  void environment_detail(bool horizon,uint8_t cloud,uint8_t ground) { detail={horizon,cloud,ground,true}; }
  std::vector<int16_t> take_audio();
  void click_sound(){audio.play(17);}
  void audio_clock(uint64_t ticks){audio.advance_to(ticks);}
  void audio_observer(audio_event_sink sink){audio.observe=std::move(sink);}
  pcm_sound_bank const &audio_state()const{return audio.state();}
  std::optional<vec2<int32_t>> take_pointer_warp();
  std::vector<framebuffer> take_pan_frames();
  framebuffer const &image() const;
  palette const &colours() const;
};

} // namespace earthsiege
