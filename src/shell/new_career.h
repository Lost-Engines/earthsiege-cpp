#pragma once
#include "shell/campaign_return.h"
#include "resources/resource_store.h"
namespace earthsiege {
technology_state read_technology(bytes source, game_edition edition=game_edition::original, int16_t battle_set=0);
crew_member make_crew_member(random_state *random=nullptr);
struct campaign_start {
  career_record career;
  std::array<hangar,41> hangars;
  /// The constructor writes the current inventory into every history block;
  /// its in-memory inactive inventories remain empty until a file is loaded.
  std::vector<uint8_t> initial_career_file() const;
};
campaign_start create_career(resource_store const &resources,int16_t battle_set,int16_t repair_policy,random_state &random, uint8_t difficulty=1);
} // namespace earthsiege
