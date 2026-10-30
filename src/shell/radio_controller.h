#pragma once
#include "simulation/radio_queue.h"
#include <functional>

namespace earthsiege {
struct radio_presentation_state {
  std::optional<size_t> active;
  bool starting{}, ready{}, cancelling{}, visible{};
};
struct radio_presentation_callbacks {
  std::function<void(radio_message const &)> start;
  std::function<bool(radio_message const &)> show;
  std::function<bool(radio_message const &)> finish;
};
/// Original active-message consumer. Portrait/speech completion and the ability
/// to paint during camera transitions are explicit presentation callbacks.
class radio_controller {
  radio_queue messages;
  radio_presentation_state data;
public:
  radio_queue &queue();
  radio_queue const &queue() const;
  radio_presentation_state const &state() const;
  void ready();
  void cancel();
  bool cancel_message(uint16_t id,std::optional<size_t> speaker);
  radio_message const *active() const;
  void advance(uint32_t ticks, bool text_enabled, radio_presentation_callbacks const &callbacks);
};
} // namespace earthsiege
