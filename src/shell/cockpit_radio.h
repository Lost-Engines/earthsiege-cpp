#pragma once
#include "resources/cockpit.h"
#include "render/radio_banner.h"
#include "render/radio_portrait.h"
#include "render/wingmate_portrait.h"
#include "resources/radio_catalogue.h"
#include "shell/pilot_transmission.h"
#include "shell/radio_controller.h"
#include "simulation/mission_session.h"

namespace earthsiege {
/// External timer reads and PCM-driver results; replay never supplies radio state.
enum class radio_input_point : uint8_t { portraits, pilot_queue, system_queue, transmission, enqueue, animation, speech_playback };
using radio_input_source=std::function<uint32_t(radio_input_point,uint32_t)>;

/// Connect radio replies to the original timer, pilot portraits and text overlay.
class cockpit_radio {
  resource_store const &resources;
  random_state &random;
  radio_catalogue catalogue;
  radio_controller controller,system_controller;
  radio_input_source observation;
  std::function<bool(std::string_view)> speech_player;
  std::function<void(uint16_t,bool)> sound_player;
  void play_sound(uint16_t id,bool idle_only=false);
  uint32_t read_input(radio_input_point point,uint32_t fallback)const;
  uint32_t system_ticks{};
  bool system_blink{},system_enabled{true},system_repaint_pending{};
  std::vector<uint8_t> system_erased;
  std::string system_text;
  rectangle system_bounds;
  int system_origin_x{},system_origin_y{};
  radio_banner banner;
  wingmate_portrait labels;
  struct member {
    size_t actor,pilot;
    rectangle bounds,painted;
    radio_portrait portrait;
    std::optional<pilot_transmission> transmission;
    size_t noise_frame{};
    bool initialized{};
    std::string speech;
  };
  std::vector<member> members;
  std::vector<radio_speaker> speakers;
  rectangle parent;
  std::array<rectangle,3> portrait_bounds;
  framebuffer cache;
  bool visible{},text_enabled{true};
  std::string text;
  std::optional<size_t> text_speaker;
  std::vector<uint16_t> sounds;
  std::vector<std::string> speech_requests;
  std::optional<size_t> speaker_index(std::optional<size_t> actor)const;
  void advance_portraits(mission_session &session,uint32_t ticks);
public:
  cockpit_radio(resource_store const &resources,cockpit_layout const &layout,mission_session const &session,random_state &ui_random,bool reduced_detail=false);
  void sound_output(std::function<void(uint16_t,bool)> player){sound_player=std::move(player);}
  void speech_output(std::function<bool(std::string_view)> player){speech_player=std::move(player);}
  void input_source(radio_input_source source){observation=std::move(source);}
  radio_presentation_state const &system_state()const{return system_controller.state();}
  void captions(bool enabled){text_enabled=enabled;}
  bool submit(radio_event event,uint32_t ticks);
  void update(mission_session &session,camera_state const &camera,uint32_t ticks,int origin_x,int origin_y,bool restoring_cockpit=false);
  void prepare(mission_session &session,camera_state const &camera,uint32_t ticks,int origin_x,int origin_y,bool restoring_cockpit=false);
  void advance_queue(mission_session &session,uint32_t ticks);
  void draw(framebuffer &frame);
  void repaint(){system_repaint_pending=true;}
  void overpaint(rectangle const &bounds);
  radio_presentation_state const &state()const;
  std::vector<uint16_t> take_sounds();
  std::vector<std::string> take_speech();
};
} // namespace earthsiege
