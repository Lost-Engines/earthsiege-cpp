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
/// Connect radio replies to the original timer, pilot portraits and text overlay.
class cockpit_radio {
  resource_store const &resources;
  random_state &random;
  radio_catalogue catalogue;
  radio_controller controller;
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
  void captions(bool enabled){text_enabled=enabled;}
  bool submit(radio_event event,uint32_t ticks);
  void update(mission_session &session,camera_state const &camera,uint32_t ticks,int origin_x,int origin_y);
  void draw(framebuffer &frame)const;
  radio_presentation_state const &state()const;
  std::vector<uint16_t> take_sounds();
  std::vector<std::string> take_speech();
};
} // namespace earthsiege
