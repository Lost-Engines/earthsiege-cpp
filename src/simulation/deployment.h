#pragma once
#include <functional>
#include "simulation/frame_time.h"
#include "simulation/random_state.h"
#include "simulation/model_math.h"
namespace earthsiege {
struct deployment_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  vec3<int16_t> velocity;
  bool landed{},obstructed{},warned{};
  int32_t clock{};
  uint16_t cell{};
};
struct deployment_events {
  std::function<void(uint16_t)> sound;
  std::function<bool(vec3<int32_t> const &)> impact;
  std::function<void(vec3<int32_t> const &,bool)> finish;
};
deployment_state create_deployment(vec3<int32_t> const &destination,random_state &random);
bool advance_deployment(deployment_state &state,uint16_t opening_frames,frame_ratio ratio,deployment_events const &events);
struct deployment_approach {uint16_t heading;int32_t distance;bool avoid_actors;bool pod;};
deployment_approach choose_deployment(int16_t mode,random_state &random);
vec3<int32_t> deployment_destination(deployment_approach const &approach,vec3<int32_t> const &player,uint16_t heading,std::function<bool(vec3<int32_t> const &)> const &occupied);
}
