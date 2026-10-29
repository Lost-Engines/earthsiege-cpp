#include "hangar_artwork.h"
namespace earthsiege {
hangar_artwork::hangar_artwork(resource_store const &resources)
  :weapon_icons_{read_bitmaps(resources.read("shell0","res32/allweap.dba"))}{
  constexpr std::array names{"road","guns","rapt","toma","patr","pnth","herc","coll"};
  for(size_t i{};i<names.size();++i){
    auto read=[&](char const *suffix){return read_bitmaps(resources.read("shell0","res32/"+std::string{names[i]}+suffix+".dba"));};
    weapons_[i]=read("wep1");damaged_[i]=read("wepd");outlines_[i]=read("outl");
  }
}
void hangar_artwork::draw(framebuffer &frame,player_herc const &h,armament_catalog const &catalog,int selected_mount)const {
  // GO's point constructor at 0x25090 and painter order at 0x803d8.
  auto const ch{static_cast<size_t>(h.chassis)};auto const &bank{weapons_.at(ch)};
  frame.image(bank.at(0),origin[ch][0],origin[ch][1],true);
  for(size_t j{};j<h.weapons.size();++j){
    auto const mount{order[ch].at(j)};auto const weapon{h.weapons.at(static_cast<size_t>(mount))};
    auto const record{catalog.find(h.chassis,mount,weapon)};
    if(record&&weapon!=17){
    bool const damaged{h.condition.equipment.at(static_cast<size_t>(mount))<51};
    auto const &pixels{damaged?damaged_[ch]:bank};auto const image{static_cast<int>(record->image)-(damaged?1:0)};
    // HERC.DAT names GL images 48..50, beyond HERCWEP1's 48 images.
    // Keep the equipment; safely omit only the unavailable image.
    if(image>=0&&static_cast<size_t>(image)<pixels.size())frame.image(pixels[static_cast<size_t>(image)],record->hit_x,record->hit_y,true);
    if(mount==selected_mount&&static_cast<size_t>(record->image)<outlines_[ch].size())
      frame.image(outlines_[ch][static_cast<size_t>(record->image)],record->draw_x,record->draw_y,true);
    }
    if(ch==7&&mount==3&&(h.weapons.at(2)!=17||h.weapons.at(3)!=17))frame.image(bank.at(43),86,25,true);
  }
}
void hangar_artwork::weapon(framebuffer &frame,int index,rectangle const &bounds)const {
  if(index<0||static_cast<size_t>(index)>=weapon_icons_.size())return;
  auto const &b{weapon_icons_[static_cast<size_t>(index)]};
  frame.image(b,(bounds.left+bounds.right-static_cast<int>(b.width))/2,(bounds.top+bounds.bottom-static_cast<int>(b.height))/2,true);
}
}
