#pragma once
#include "resources/resource_store.h"
#include "simulation/radio_queue.h"
#include "simulation/random_state.h"
#include <span>

namespace earthsiege {
struct radio_speaker {size_t actor{}; uint8_t voice{1}; bool leader{};};
struct radio_text {std::string text; radio_descriptor descriptor;};
struct radio_resolution {uint32_t text{};radio_descriptor descriptor;};
class radio_catalogue {
  std::array<std::vector<radio_text>,5> banks;
public:
  explicit radio_catalogue(resource_store const &resources);
  /// Unknown speakers fall back to the first registered wingmate, as in DBSIM.
  std::optional<radio_resolution> resolve(radio_event &event, std::span<radio_speaker const> speakers, random_state &random) const;
  radio_text const &at(uint32_t text) const;
};
} // namespace earthsiege
