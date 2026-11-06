#include "simulation/mission_session.h"
#include "simulation/deployment.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
int16_t mission_session::blast_radius(size_t const id) const {
  if(id<crew.size())return crew[id].definition->profile.collision_radius;
  if(id<base_id(0))return 0;
  auto const &profile{bases.at(id-base_id(0)).actor->profile()};return profile.collision_enabled?profile.contact_radius:0;
}
void mission_session::place_deployed(size_t const id,vec3<int32_t> const &position,std::optional<uint16_t> const heading){
  if(id<crew.size()){
    auto angles{crew[id].actor->body().movement().angles};if(heading)angles.z=*heading;crew[id].actor->deploy(position,angles);
  }else if(id<base_id(0)){
    auto &motion{flyers.at(id-flyer_id(0)).actor->motion()};motion.position=position;if(heading)motion.angles.z=*heading;
  }else{
    auto &actor{*bases.at(id-base_id(0)).actor};auto angles{transform_angles(actor.placement())};if(heading)angles.z=*heading;actor.deploy(position,angles);
  }
}
bool mission_session::activate_group(size_t const index){
  auto const mode{deployment_modes.at(index)};
  if(mode<2||mode>5)return true;
  auto const &group{activity->group(index)};
  if(group.members.empty())return true;
  if(deployment_started[index])return false;
  deployment_started[index]=true;
  auto const approach{choose_deployment(mode,random)};
  auto const observations{observe()};
  std::vector<scenery_collider> buildings;
  for(auto const &member:bases)if(auto field{member.actor->occupancy()})buildings.push_back(*field);
  auto const destination{deployment_destination(approach,player().movement().position,player().movement().angles.z,[&](vec3<int32_t> const &point){
    if(approach.avoid_actors)for(size_t id{};id<activity->size();++id){
      auto const radius{blast_radius(id)};if(radius&&!activity->waiting(id)&&fast_distance(point-observations[id].position)<radius+5000)return true;
    }
    return scenery_contact_height(buildings,point)!=0||scenery_contact_height(terrain.colliders(),point)!=0;
  })};
  if(approach.pod){effects.add_deployment(index,destination,random);return false;}
  auto const heading{static_cast<uint16_t>(approach.heading-0x8000)};
  place_deployed(group.members.front(),destination,heading);
  for(size_t ordinal{1};ordinal<group.members.size();++ordinal){
    auto const id{group.members[ordinal]};auto const &offset{layout.formation_offset(group.kind,group.formation,activity->actor(id).member-1)};
    auto position{destination+rotate_vector(offset,rotation_transform({0,0,heading}))};
    place_deployed(id,position);
  }
  return true;
}
}
