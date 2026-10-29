#pragma once
#include "render/damage_display.h"
#include "resources/combat.h"
#include "resources/weapon_mounts.h"

namespace earthsiege {
struct monitored_weapon {int16_t definition{-1},damage{};std::string name;};
struct full_damage_state {
  int16_t model{-1};
  std::array<int16_t,19> armour{};
  std::array<int16_t,12> internal{};
  std::vector<monitored_weapon> weapons; // original GL slot order, including empty slots
  std::string subject_name;
  std::vector<std::string> weapon_labels; // retained original shared label table, when supplied
  int mode{},scroll{},subject{};
  bool no_information{},failed{};
};
struct weapon_diagram {int frame{-1},x{},y{};rectangle bounds;int flags{};size_t slot{};};
/// Original lower-panel structural, internal and weapon damage diagrams.
class full_damage_display {
  struct chassis_art {paper_doll diagram;std::vector<bitmap> frames;std::vector<weapon_mount> mounts;};
  std::vector<chassis_art> chassis;
  std::vector<bitmap> weapon_art;
  std::vector<std::array<int32_t,2>> weapon_sizes;
  weapon_catalogue definitions;
  std::array<font,5> health_fonts;
  font yellow,red;
  interface_strings strings;
public:
  explicit full_damage_display(resource_store const &resources);
  std::vector<weapon_diagram> diagrams(full_damage_state const &state)const;
  void draw(framebuffer &target,rectangle const &bounds,rectangle const &listing,full_damage_state const &state)const;
};
} // namespace earthsiege
