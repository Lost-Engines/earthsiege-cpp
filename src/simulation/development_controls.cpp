#include "development_controls.h"
#include "simulation/model_math.h"
#include "simulation/fixed_math.h"
#include "simulation/frame_time.h"
#include <algorithm>
#include <array>
namespace earthsiege {
bool development_controls::reposition(uint16_t code,vec3<int32_t> &position,vec3<uint16_t> &angles){
  if(code>=0x602&&code<=0x60a){
    constexpr std::array<int16_t,9> steps{500,1000,1500,2000,3000,4500,6000,7500,9000};
    translation_step=turn_step=steps[code-0x602];return true;
  }
  if(code==0x44b||code==0x44d){angles.z=static_cast<uint16_t>(angles.z+(code==0x44b?turn_step:-turn_step));return true;}
  vec3<int16_t> delta;
  switch(code){
    case 0x24b:delta.x=wrap_word(-translation_step);break;
    case 0x24d:delta.x=translation_step;break;
    case 0x248:delta.y=translation_step;break;
    case 0x250:delta.y=wrap_word(-translation_step);break;
    default:return false;
  }
  auto transform{rotation_transform(angles)};transform.translation=position;
  position=transform_point(delta,transform);return true;
}
vec3<int32_t> development_pod_destination(vec3<int32_t> const &position,uint16_t heading){
  return position+rotate_vector({0,50000,0},rotation_transform({0,0,heading}));
}
int development_damage_passes(std::span<development_contact const> contacts,vec3<int32_t> const &position,uint8_t side,bool safe){
  if(safe)return 0;
  int32_t friendly{},opposing{};
  for(auto const &actor:contacts){
    if(!actor.eligible)continue;
    vec3<int32_t> delta;
    for(unsigned axis{};axis<3;++axis)delta[axis]=wrap_long(static_cast<int64_t>(actor.position[axis])-position[axis]);
    if(fast_distance(delta)>=actor.radius)continue;
    (actor.side==side?friendly:opposing)+=actor.strength;
  }
  friendly=std::max(0,friendly-600);
  return opposing>friendly?(opposing-friendly+399)/400:0;
}
}
