#pragma once
#include "shell/mission_launch.h"
#include "resources/campaign.h"
#include "resources/mission_text.h"
#include "simulation/mission_session.h"
namespace earthsiege {
/// Owns the GO-to-DBSIM handoff independently of windows and host files.
class game_state {
  resource_store const &resources;
  random_state random;
  std::optional<campaign_start> career_;
  single_mission_crew single_;
  std::optional<prepared_mission> prepared_;
  player_roster player_;
  std::string mission_name_;
  mission_text text_;
  bool campaign_{};
  int16_t identity_{};
  int16_t last_grade_{},last_kind_{};
  void prepare(std::string name,mission_options const &options);
public:
  explicit game_state(resource_store const &resources,int seed=17);
  bool campaign() const{return campaign_;}
  bool ready() const{return prepared_.has_value();}
  bool has_career() const{return career_.has_value();}
  std::string const &mission_name()const{return mission_name_;}
  mission_text const &text()const{return text_;}
  prepared_mission const &prepared()const{return prepared_.value();}
  player_roster const &player()const{return player_;}
  random_state::state const &random_snapshot()const{return random.snapshot();}
  campaign_start const &career()const{return career_.value();}
  career_state &crew();
  career_state const &crew()const;
  armory &inventory();
  armory const &inventory()const;
  hangar &roster();
  hangar const &roster()const;
  void new_campaign(int16_t battle_set=0,int16_t repair_policy=1,int16_t identity=0);
  void load_campaign(campaign_start saved,int16_t identity=0);
  void prepare_single(int16_t category,mission_options const &options);
  void prepare_campaign(bool next=false,mission_options const &options={});
  void restore_history(size_t history);
  void update_roster();
  void checkpoint(size_t history=0);
  std::unique_ptr<mission_session> launch();
  /// GO is a fresh process after flight: restore its saved career and RNG.
  void resume_shell(int16_t seed,std::optional<campaign_start> saved={});
  void complete(mission_results const &results,mission_counters const &counters);
};
}
