#include "deployment.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
deployment_state create_deployment(vec3<int32_t> const &destination,random_state &random){
  deployment_state s;
  s.angles={0,0,static_cast<uint16_t>(random.next())};
  auto const rotation{rotation_transform(s.angles)};
  auto const distance{70000+random.bounded(25000)};
  s.position=rotate_vector({0,-distance,0},rotation)+destination;
  auto const steps{distance/2000};s.position.z=((steps*steps)>>1)*50;
  auto const velocity{rotate_vector({0,2000,0},rotation)};
  s.velocity={wrap_word(velocity.x),wrap_word(velocity.y),wrap_word(-s.position.z/steps)};
  return s;
}
bool advance_deployment(deployment_state &s,uint16_t const frames,frame_ratio const ratio,deployment_events const &events){
  if(!s.landed){
    s.position+=vec3<int32_t>{s.velocity};
    s.angles.x=direction_angle(2000,s.velocity.z);
    if(!s.warned&&s.position.z<50000){if(events.sound)events.sound(47);s.warned=true;}
    if(s.position.z<0){s.position.z=0;s.landed=true;s.angles.x=0;if(events.sound)events.sound(48);if(events.impact)s.obstructed=events.impact(s.position);}
  }else{
    auto const frame{s.clock>>10};
    if(frame>=frames){if(events.finish)events.finish(s.position,!s.obstructed);return true;}
    s.cell=static_cast<uint16_t>(frame);s.clock+=scale_frame_time(1500,ratio);
  }
  return false;
}
deployment_approach choose_deployment(int16_t const mode,random_state &random){
  int const draw{random.next()};int heading{};
  if(mode==2)heading=0x4000-(draw&0x7fff);
  else if(mode==3)heading=0x1000-(draw&0x1fff);
  else if(mode==4)heading=0x1000-(draw&0x1fff)-0x8000;
  else heading=0x2000-(draw&0x3fff);
  return{static_cast<uint16_t>(heading),mode==4?70000:100000,mode>=4,mode<4};
}
vec3<int32_t> deployment_destination(deployment_approach const &a,vec3<int32_t> const &player,uint16_t const heading,std::function<bool(vec3<int32_t> const &)> const &occupied){
  auto const rotation{rotation_transform({0,0,static_cast<uint16_t>(heading+a.heading)})};
  vec3<int32_t> result{};int32_t distance{a.distance};
  do{
    auto const offset{rotate_vector({0,distance,0},rotation)};
    result.x=offset.x+player.x;result.y=offset.y+player.y;result.z+=player.z;
    distance+=2000;
  }while(occupied(result));
  return result;
}
}
