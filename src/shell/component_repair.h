#pragma once
#include "shell/hangar_service.h"
namespace earthsiege {
enum class repair_group:int16_t { internal=1924,armour=1925,equipment=1926 };
int16_t component_condition(herc_condition const &,repair_group,int16_t index);
size_t component_count(player_herc const &,repair_group);
/// Original 1920 editor transaction. Its quote uses the mount index for weapons,
/// and accepting a changed budget applies the selected row's next band again.
class component_repair {
  service_prices const *prices;
  herc_condition original;
  int32_t original_budget;
  repair_group group_{repair_group::internal};
  int16_t index_{},allocated_{},tier_{},quote_{};
  int16_t quote(player_herc const &,bool increase,int16_t condition)const;
  void refresh(player_herc const &);
  void change_condition(player_herc &,int16_t)const;
  void spend(career_state &,int32_t);
public:
  component_repair(player_herc const &,career_state const &,service_prices const &);
  void select(player_herc const &,repair_group,int16_t);
  bool can_increase(career_state const &)const;
  void increase(player_herc &,career_state &);
  void decrease(player_herc &,career_state &);
  void accept(player_herc &,career_state const &)const;
  void cancel(player_herc &,career_state &);
  repair_group group()const{return group_;}
  int16_t index()const{return index_;}
  int16_t allocated()const{return allocated_;}
  int16_t tier()const{return tier_;}
  int16_t required()const{return quote_;}
};
}
