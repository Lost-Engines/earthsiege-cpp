#include "shell/mission_view.h"
#include "resources/input_tape.h"
#include "simulation/model_math.h"
#include <iostream>
using namespace earthsiege;
namespace{
void check(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
auto positions(mission_session const &s){std::vector<vec3<int32_t>> result;for(auto const &h:s.hercs())result.emplace_back(h.actor.body().movement().position);return result;}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};bool const expansion{resources.edition()==game_edition::expansion};
 auto tape{read_input_tape(read_file(resources.root()/"TAPES/DEMO1.TAP"),resources.edition())};
 auto owner{std::make_unique<mission_session>(resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)],tape.edition),random_state{},read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)]))};
 auto &s{*owner};mission_view view{resources,std::move(owner)};
 s.playback(false);check(s.player_command(0x21f)&&!s.recording_paused(),"Alt+S ignored without tape owner");
 size_t drawn{};view.observe_scene([&](auto const &){++drawn;});view.draw();check(drawn>0,"normal scene draws models");drawn=0;
 s.playback(true);check(s.player_command(0x21f)&&s.recording_paused(),"Alt+S enables world freeze during playback");
 view.draw();check(drawn==0,"frozen playback retains the scene instead of repainting world geometry");
 // Input remains live while world motion is frozen, including throttle changes.
 for(int mode{};mode!=3;++mode){
  if(mode)check(s.player_command(0x402),"direction command accepted");
  s.player_command(76);auto before{positions(s)};
  player_input input;input.movement.y=!expansion&&mode==2?-256:256;
  check(s.step_player(input,1024).has_value(),"input phase continues during freeze");
  auto throttle{s.player().movement().throttle};
  check(throttle==(!expansion&&mode==1?0:-145),"Ctrl+1 changes limits only in base edition");
  check(positions(s)==before,"frozen input does not integrate actor positions");
 }
 s.player_command(76);
 check(s.player_command(0x605),"translation step selected");
 auto before{s.player().movement().position};auto angle{s.player().movement().angles};
 check(s.player_command(0x248),"direct movement command accepted");
 auto expected{before+rotate_vector({0,2000,0},rotation_transform(angle))};
 check(s.player().movement().position==expected,"direct movement remains available while frozen");
 check(s.player_command(0x44b),"direct rotation command accepted");
 check(s.player().movement().angles.z==static_cast<uint16_t>(angle.z+2000),"Ctrl+Left turns the HERC");
 // Unfreezing from the input phase does not retroactively run the world phase.
 auto frozen_positions{positions(s)};player_input resume;resume.command=0x21f;
 check(s.step_player(resume,1024).has_value()&&!s.recording_paused(),"resume command consumed");
 check(positions(s)==frozen_positions,"world phase remains skipped in resume frame");
 s.player_command(0x21f);
 view.change_camera(camera_mode::orbit);tape_frame frame;frame.delta=1;frame.command=49;
 check(view.step_recorded(frame,0),"squad view command");auto const target{view.camera_actor()};
 auto const player_pos{s.player().movement().position};auto const p{s.hercs()[target].actor.body().movement().position};auto const a{s.hercs()[target].actor.body().movement().angles};
 frame.command=0x24b;check(view.step_recorded(frame,0),"viewed-actor movement command");
 check(s.hercs()[target].actor.body().movement().position==p+rotate_vector({-2000,0,0},rotation_transform(a)),"external view routes direct movement to viewed HERC");
 if(target!=s.player_id())check(s.player().movement().position==player_pos,"squad reposition preserves player position");
 if(expansion){
  auto bytes{read_file(argv[2])};binary_reader r{bytes};auto magic{r.take(8)};check(std::string(magic.begin(),magic.end())=="ESDEVP01","pod fixture version");auto count{r.integer<uint32_t>()};
  for(unsigned i{};i<count;++i){auto heading{r.integer<uint16_t>()};vec3<int32_t> destination{r.integer<int32_t>(),r.integer<int32_t>(),r.integer<int32_t>()};
   check(development_pod_destination({1000,2000,3000},heading)==destination,"original global drop-pod destination");}
  r.require_end();
  world_effects effects{resources};random_state random{17};size_t impacts{},releases{};
  for(auto group:{std::optional<size_t>{},std::optional<size_t>{3}}){
   check(effects.add_deployment(group,{0,0,0},random),"deployment created");
   for(int i{};i<200;++i){effects.advance_deployments(1024,[&](auto const &){++impacts;return false;},[&](size_t g,auto const &){check(g==3,"assigned deployment group");++releases;},{0,0,0});effects.collect();}
   check(effects.ground_flashes().size()==(group?1:0),"debug pod leaves no crater; assigned pod does");
  }
  check(impacts==2&&releases==1,"both pods impact, only assigned pod releases a HERC");
  auto &live{s.visual_effects()};std::vector<scene_model> models;live.append_models(models);auto size{models.size()};
  check(s.player_command(0x619),"expansion development pod command");models.clear();live.append_models(models);check(models.size()==size+1,"global pod command creates visible deployment effect");
 }else check(s.player_command(0x619),"base damage command accepted");
 std::cout<<(expansion?"Expansion":"Base")<<" development controls, recording freeze, input phase order and squad routing pass\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
