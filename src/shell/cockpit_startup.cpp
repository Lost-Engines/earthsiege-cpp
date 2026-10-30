#include "cockpit_startup.h"
#include <algorithm>
namespace earthsiege {
cockpit_startup::cockpit_startup(palette const &target):source_{target},colours_{target}{
  // 57cf4's dim cockpit banks deliberately use G/2, R/2, R/2.
  for(unsigned first:{32U,64U})for(unsigned i{first};i<first+16;++i){
    auto const &c{target.dac_colours[i]};
    source_.dac_colours[i]={static_cast<uint8_t>(c[1]/2),static_cast<uint8_t>(c[0]/2),static_cast<uint8_t>(c[0]/2)};
  }
  for(size_t i{};i<256;++i){
    for(unsigned c{};c<3;++c)increments_[i][c]=static_cast<int16_t>((static_cast<int>(target.dac_colours[i][c])-source_.dac_colours[i][c])*256/120);
    source_.colours[i]=display_colour(source_.dac_colours[i]);
  }
  colours_=source_;
}
int cockpit_startup::cover_y(int32_t camera_height,camera_projection const &projection)const{
  return project_point({0,1000,-camera_height},projection,projection_source::model).y;
}
void cockpit_startup::prepare_frame(random_state &random){
  if(lifting()||finished_)return;
  int const amount{static_cast<uint16_t>(random.next())%5};
  int const below{displacement_},above{5-displacement_};
  displacement_=static_cast<int16_t>(displacement_+(above>below?std::min(above,amount):-std::min(below,amount)));
}
void cockpit_startup::update_palette(uint32_t now,int cover){
  if(!lifting()||!fading_||cover<=-200)return;
  if(!fade_expiry_)fade_expiry_=now+120;
  if(now>*fade_expiry_){fading_=false;return;}
  int const elapsed{120-static_cast<int>(*fade_expiry_-now)};
  for(size_t i{};i<256;++i){
    for(unsigned c{};c<3;++c)colours_.dac_colours[i][c]=static_cast<uint8_t>(source_.dac_colours[i][c]+((increments_[i][c]*elapsed)>>8));
    colours_.colours[i]=display_colour(colours_.dac_colours[i]);
  }
  // No final snap to the target: preserve the original fixed-point remainder.
  if(now==*fade_expiry_)fading_=false;
}
void cockpit_startup::presented(uint32_t now){
  if(!lifting()||finished_)return;
  if(++lift_==115)shake_expiry_=now+30;
}
void cockpit_startup::drawn(uint32_t now){
  if(!lifting()&&now>=shake_expiry_){finished_=true;displacement_=0;}
}
} // namespace earthsiege
