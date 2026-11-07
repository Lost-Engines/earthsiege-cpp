#include "shell/demo_playback.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=4&&argc!=5)throw std::runtime_error{"Usage: trace_demo_path DATA DEMO OUTPUT.jsonl [MAX_FRAMES]"};
 resource_store resources{argv[1]};unsigned const demo{static_cast<unsigned>(std::stoul(argv[2]))};
 std::filesystem::path const output{argv[3]};std::filesystem::create_directories(output.parent_path());
 std::ofstream trace{output};if(!trace)throw std::runtime_error{"Cannot open trace output"};
 preferences::values defaults{1,1,2,1,255,255,255,0,0,0,0,1,0,0,2,0,0,1,0,0,0,0};
 preferences prefs{defaults,true,resources.edition()};
 demo_playback playback{resources,demo,prefs,output.parent_path()/"replay-settings"};
 auto emit=[&](char const *event,size_t frame){
  auto const &s{playback.view().simulation().player().movement()};
  trace<<"{\"event\":\""<<event<<"\",\"frame\":"<<frame<<",\"packets\":"<<playback.packets_read()
   <<",\"position\":["<<s.position.x<<','<<s.position.y<<','<<s.position.z<<"],\"angles\":["<<s.angles.x<<','<<s.angles.y<<','<<s.angles.z
   <<"],\"throttle\":"<<s.throttle<<",\"gait_rate\":"<<s.gait_rate<<",\"steering_rate\":"<<s.steering_rate
   <<",\"playback_rate\":"<<s.playback_rate<<",\"collision_recovery\":"<<s.collision_recovery<<",\"reverse_recovery\":"<<s.reverse_recovery;
  auto const &sim{playback.view().simulation()};
  trace<<",\"random\":[";auto const &rng{sim.random_snapshot()};
  for(auto v:rng.words)trace<<v<<',';
  trace<<+rng.first<<','<<+rng.second<<"],\"hercs\":[";
  bool first{true};for(auto const &h:sim.hercs()){
    if(!first)trace<<',';
    first=false;auto const &m{h.actor.body().movement()};
    trace<<'['<<m.position.x<<','<<m.position.y<<','<<m.position.z<<','<<m.angles.x<<','<<m.angles.y<<','<<m.angles.z<<','<<m.throttle<<','<<m.gait_rate<<','<<m.steering_rate<<']';
  }
  trace<<"],\"flyers\":[";first=true;
  for(auto const &f:sim.flying_actors()){
    if(!first)trace<<',';
    first=false;auto const &m{f.actor.motion()};
    trace<<'['<<m.position.x<<','<<m.position.y<<','<<m.position.z<<','<<m.angles.x<<','<<m.angles.y<<','<<m.angles.z;
    trace<<','<<m.velocity.x<<','<<m.velocity.y<<','<<m.velocity.z<<','<<m.angular_step.x<<','<<m.angular_step.y<<','<<m.angular_step.z<<']';
  }
  trace<<"],\"aim\":[";first=true;
  for(auto const &h:sim.hercs()){
    if(!first)trace<<',';
    first=false;auto const &body{h.actor.body()};
    trace<<'['<<body.torso().rate<<','<<body.elevation().rate<<','<<body.torso().angle<<','<<body.elevation().angle<<']';
  }
  trace<<"],\"power\":[";first=true;
  for(auto const &h:sim.hercs()){
    if(!first)trace<<',';
    first=false;auto const &power{h.actor.power()};
    trace<<'['<<power.reserve<<','<<power.shields.front<<','<<power.shields.rear<<','<<power.shields.balance<<','<<power.shields.capacity<<']';
  }
  trace<<"],\"damage\":[";first=true;
  for(auto const &h:sim.hercs()){
    if(!first)trace<<',';
    first=false;auto const &d{h.actor.damage()};
    trace<<'[';bool first_part{true};
    for(auto const *parts:{&d.internal,&d.armour,&d.present}){
      if(!first_part)trace<<',';
      first_part=false;trace<<'[';bool first_value{true};
      for(auto v:*parts){if(!first_value)trace<<',';first_value=false;trace<<+v;}trace<<']';
    }
    trace<<']';
  }
  trace<<"],\"awareness\":[";first=true;
  for(size_t i{};i<sim.activity_state().size();++i){
    if(!first)trace<<',';
    first=false;auto const &a{sim.activity_state().actor(i).awareness};
    trace<<'['<<a.visibility_remaining<<','<<a.refresh_remaining<<','<<a.seen<<','<<a.alerted<<",[";
    for(size_t j{};j<a.known.size();++j){if(j)trace<<',';trace<<+a.known[j];}
    trace<<"],[";
    for(size_t j{};j<a.visible.size();++j){if(j)trace<<',';trace<<+a.visible[j];}
    trace<<"]]";
  }
  trace<<"],\"strengths\":[";first=true;
  for(auto const &h:sim.hercs()){
    if(!first)trace<<',';
    first=false;trace<<h.actor.combat_rating();
  }
  trace<<"],\"attackers\":[";first=true;
  for(auto const &h:sim.activity_state().targets()){
    if(!first)trace<<',';
    first=false;trace<<h.attackers;
  }
  trace<<"],\"controllers\":[";first=true;
  auto const &activity{sim.activity_state()};
  for(size_t i{};i<activity.size();++i){
    if(!first)trace<<',';
    first=false;auto const &decision{activity.decisions()[i]};
    trace<<'['<<(decision.controller.type?static_cast<int>(*decision.controller.type):-1)<<','<<decision.controller.reconsider_remaining<<','<<(activity.targets()[i].selected?static_cast<int>(*activity.targets()[i].selected):-1)<<','<<decision.command<<']';
  }
  trace<<"]}\n";
 };
 size_t const limit{argc==5?std::stoul(argv[4]):20000};
 while(!playback.done()&&playback.frames_played()<limit){
  auto const frame{playback.frames_played()};bool const startup{playback.view().starting()};if(!startup)emit("begin",frame);
  playback.step([&](unsigned,framebuffer const &){playback.view().take_audio();playback.view().take_pan_frames();return true;});
  if(playback.frames_played()!=frame)emit("end",frame);
 }
 std::cout<<"Demo "<<demo<<": "<<playback.frames_played()<<" frames, "<<playback.packets_read()<<'/'<<playback.packet_count()<<" packets, "<<playback.modal_packets_read()<<" modal packets\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
