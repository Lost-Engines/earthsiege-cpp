#pragma once
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "shell/armament.h"
namespace earthsiege {
/// Original bay bitmap layers. Resident TTM programs are composed separately.
class hangar_artwork {
  std::array<std::vector<bitmap>,8> weapons_,damaged_,outlines_,construction_;
  std::vector<bitmap> weapon_icons_;
  struct damage_layer {int16_t image;int32_t x,y;};
  std::array<std::vector<bitmap>,8> damage_images_;
  std::array<std::array<std::array<std::vector<damage_layer>,3>,6>,8> damage_layers_;
public:
  static constexpr std::array<std::array<int,2>,8> origin{{{77,99},{77,89},{83,64},{86,53},{75,57},{73,37},{72,37},{58,26}}};
  static constexpr std::array<std::array<int,9>,8> order{{{1,2,0},{1,2,0},{3,2,1,0},{4,3,2,1,0},{5,2,4,1,3,0},{1,6,4,3,5,2,0},{1,6,4,3,7,5,2,0},{1,8,3,6,4,2,5,7,0}}};
  explicit hangar_artwork(resource_store const &resources);
  void draw(framebuffer &frame,player_herc const &herc,armament_catalog const &catalog,int selected_mount=-1)const;
  void body(framebuffer &frame,player_herc const &herc,int remaining=0)const;
  void equipment(framebuffer &frame,player_herc const &herc,armament_catalog const &catalog,int selected_mount=-1)const;
  void damage(framebuffer &frame,player_herc const &herc)const;
  void weapon(framebuffer &frame,int index,rectangle const &bounds)const;
};
}
