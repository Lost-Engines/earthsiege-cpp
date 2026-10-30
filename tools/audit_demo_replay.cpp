#include <iostream>
#include <algorithm>
#include <string>
#include "shell/mission_view.h"
#include "shell/mission_options_dialog.h"
#include "shell/mission_alert.h"
#include "resources/resource_store.h"
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc<3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader file{raw};
 auto magic{file.take(8)};auto version{std::string(magic.begin(),magic.end())};bool terminal{version=="ESRPLY05"},extended{terminal||version=="ESRPLY04"};
 if(version!="ESRPLY03"&&!extended)file.fail("format");
 auto demo{file.integer<uint32_t>()},count{file.integer<uint32_t>()};
 auto prefs{extended?file.take(17):bytes{}};
 auto expected_packets{terminal?file.integer<uint32_t>():uint32_t{0}};
 auto tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
 mission_view view{resources,tape};
 preferences settings{prefs};std::array<joystick_calibration,2> calibrations;
 if(extended)view.configure_input(settings,calibrations);
 save_store replay_saves{std::filesystem::path{argv[2]}.parent_path()/"replay-settings"};
 size_t next_packet{},modal_packets{},alert_count{};bool suppressed{},exited{};
 auto alert=[&](int message){
  mission_alert dialog{resources,message};
  if(next_packet>=tape.frames.size())throw std::runtime_error{"Missing alert initialization poll"};
  ++next_packet;++modal_packets;suppressed=true;
  bool left{};std::optional<int> response;
  while(!response){
   if(next_packet>=tape.frames.size())throw std::runtime_error{"Recording ended inside alert"};
   auto const &input{tape.frames[next_packet++]};++modal_packets;
   // 4cb14 dispatches queued mouse callbacks before the modal key handler.
   for(auto const &event:input.mouse){
    bool const held{bool(event.kind&1)};
    if(held&&!left)dialog.mouse_down(event.position.x,event.position.y);
    if(!held&&left)response=dialog.mouse_up(event.position.x,event.position.y);
    left=held;
   }
   if(!response)response=dialog.input(input.command,input.buttons&1,input.buttons&2,settings.current()[10]!=0).response;
  }
  ++alert_count;
  std::cout<<"Alert "<<message<<" response "<<*response<<" at packet "<<next_packet-1<<'\n';
  exited=dialog.exits_mission(*response);return exited;
 };
 auto modal=[&](uint16_t code)->mission_command_result{
  if(code==16)return alert(static_cast<int>(view.status(true)))?mission_command_result::exit_mission:mission_command_result::handled;
  if(code!=88&&code!=0x219)return mission_command_result::unhandled;
  // Original 45f00 blocks buttons and consumes one initialization poll before
  // the dialog loop. Playback skips the live debounce which would clear them.
  suppressed=true;view.begin_options();
  if(next_packet>=tape.frames.size())throw std::runtime_error{"Missing modal initialization poll"};
  ++next_packet;++modal_packets;
  mission_options_dialog options{resources,settings,replay_saves,calibrations};
  bool left{},right{};
  while(!options.done()){
   if(next_packet>=tape.frames.size())throw std::runtime_error{"Recording ended inside preferences"};
   auto const &input{tape.frames[next_packet++]};++modal_packets;
   // These recordings exercise the original keyboard-owned preferences path.
   // Other input remains explicit rather than silently skipping a dialog.
   if(input.buttons)throw std::runtime_error{"Recorded preferences device buttons need routing"};
   options.key(input.command);
   for(auto const &event:input.mouse){
    bool const l{bool(event.kind&1)},r{bool(event.kind&2)};
    if(l&&!left)options.mouse_down(event.position.x,event.position.y);
    if(r&&!right)options.mouse_down(event.position.x,event.position.y,true);
    if(!l&&left)options.mouse_up(event.position.x,event.position.y);
    if(!r&&right)options.mouse_up(event.position.x,event.position.y,true);
    left=l;right=r;
   }
  }
  view.apply_preferences(settings);view.end_options();return mission_command_result::handled;
 };
 uint32_t checked{};
 for(uint32_t frame{};frame<count;++frame){
  auto phase{terminal?file.integer<uint8_t>():uint8_t{1}};
  auto packet{file.integer<int32_t>()};auto clock{file.integer<uint32_t>()};auto ratio{file.integer<uint16_t>()};auto actors{file.integer<uint16_t>()};
  if(argc>3&&frame>std::stoul(argv[3]))break;
  if(packet!=-1&&packet!=-3){
   tape_frame record;
   if(packet>=0){
    if(next_packet!=size_t(packet))file.fail("modal input ownership");
    record=tape.frames.at(next_packet++);
   }
   else record.delta=ratio; // EOF clears the packet but retains the outer ratio.
   if(record.delta!=ratio)file.fail("recorded outer ratio");
   auto const complete{view.step_recorded(record,clock,suppressed,{modal,[&](mission_status status){return alert(static_cast<int>(status));}})};
   if(terminal&&((phase==2)==complete))file.fail("mission frame termination phase");
   if(complete)view.draw();
  }
  auto const &session{view.simulation()};
  auto const &rng{session.random_snapshot()};
  random_state::state expected;for(auto &v:expected.words)v=file.integer<uint16_t>();expected.first=file.integer<uint8_t>();expected.second=file.integer<uint8_t>();
  size_t errors{};auto check=[&](std::string const &label,auto actual,auto want){
   if(actual!=want){if(errors<25)std::cerr<<"packet "<<packet<<' '<<label<<": native "<<+actual<<", original "<<+want<<'\n';++errors;}
  };
  check("random first",rng.first,expected.first);check("random second",rng.second,expected.second);
  for(size_t i{};i<rng.words.size();++i)check("random word "+std::to_string(i),rng.words[i],expected.words[i]);
  auto hercs{session.hercs()};auto flyers{session.flying_actors()};auto bases{session.ground_actors()};size_t h{},f{},b{};
  check("actor count",hercs.size()+flyers.size()+bases.size(),size_t(actors));
  for(unsigned a{};a<actors;++a){
   std::array<uint8_t,0x250> storage{};
   for(auto const [at,n]:std::array<std::pair<size_t,size_t>,10>{{{0,2},{6,6},{32,12},{0x4c,4},{0x90,1},{0x93,1},{0x9e,1},{0x1b8,4},{0x222,8},{0x22e,4}}}){
    auto part{file.take(n)};std::copy(part.begin(),part.end(),storage.begin()+at);
   }
   if(extended)for(auto const [at,n]:std::array<std::pair<size_t,size_t>,14>{{{0x50,4},{0x7c,2},{0x8f,1},{0x97,2},{0x9c,1},{0xa6,1},{0x15c,2},{0x179,2},{0x17c,2},{0x1d2,2},{0x238,2},{0x22a,4},{0xbc,actors},{0x10c,actors}}}){
    auto part{file.take(n)};std::copy(part.begin(),part.end(),storage.begin()+at);
   }
   bytes data{storage};auto get=[&]<class T>(size_t at){binary_reader r{data.subspan(at)};return r.integer<T>();};
   auto kind{get.template operator()<uint16_t>(0)};
   auto expected_target{file.integer<int32_t>()};auto expected_controller{file.integer<int16_t>()};
   auto const &relationship{session.activity_state().targets()[a]};
   auto const &controller{session.activity_state().decisions()[a].controller};
   check("actor "+std::to_string(a)+" target",relationship.selected?int32_t(*relationship.selected):-1,expected_target);
   if(extended){
    auto const &awareness{session.activity_state().actor(a).awareness};
    std::string label{"actor "+std::to_string(a)+" "};
    auto byte=[&](size_t at){return get.template operator()<uint8_t>(at)!=0;};
    check(label+"known size",awareness.known.size(),size_t(actors));
    for(size_t i{};i<actors;++i){
     check(label+"known "+std::to_string(i),awareness.known[i],get.template operator()<uint8_t>(0xbc+i));
     check(label+"visible "+std::to_string(i),awareness.visible[i],get.template operator()<uint8_t>(0x10c+i));
    }
    check(label+"seen",awareness.seen,byte(0x8f));
    check(label+"alerted",awareness.alerted,byte(0x98));
    check(label+"event busy",awareness.event_busy,byte(0x9c));
    check(label+"visibility clock",awareness.visibility_remaining,get.template operator()<int16_t>(0x179));
    check(label+"refresh clock",awareness.refresh_remaining,get.template operator()<int16_t>(0x17c));
    check(label+"attackers",relationship.attackers,get.template operator()<int16_t>(0x15c));
    check(label+"target changed",relationship.changed,byte(0x97));
    if(kind!=5){
     check(label+"controller updates",controller.ticks,get.template operator()<uint32_t>(0x50));
     check(label+"controller updated",controller.updated,byte(0x7c));
     check(label+"controller first",controller.first_entry,byte(0x7d));
    }
    if(kind==7){
     auto const &decision{session.activity_state().decisions()[a]};
     check(label+"command",decision.command,get.template operator()<int16_t>(0x1d2));
     // Original +0x238 is untouched by construction. Compare only when an
     // attacking AI consumer has chosen a component, not allocator residue.
     if(!session.activity_state().actor(a).player&&controller.type&&describe(*controller.type).attacking()&&relationship.selected&&session.activity_state().actor(*relationship.selected).kind==actor_kind::herc)
      check(label+"aimed component",decision.aimed_component,get.template operator()<int16_t>(0x238));
     check(label+"attack request",decision.attack_requested,byte(0xa6));
    }
   }
   if(kind!=5){
    check("actor "+std::to_string(a)+" controller",controller.type?int(std::to_underlying(*controller.type)):-1,int(expected_controller));
    check("actor "+std::to_string(a)+" controller clock",controller.reconsider_remaining,get.template operator()<int32_t>(0x4c));
   }vec3<int32_t> position;vec3<uint16_t> angles;
   std::string label{"actor "+std::to_string(a)+" "};
   if(kind==7){
    auto const &actor{hercs.at(h++).actor};auto const &body{actor.body()};auto const &motion{body.movement()};
    position=motion.position;angles=motion.angles;
    auto const &damage{actor.damage()};
    check(label+"dead",damage.consequences.dead,get.template operator()<uint8_t>(0x93)!=0);
    check(label+"disabled",damage.consequences.disabled,get.template operator()<uint8_t>(0x9e)!=0);
    check(label+"detectable",actor.impacts().detectable,get.template operator()<uint8_t>(0x90)!=0);
    for(auto const &values:{damage.internal,damage.armour,damage.present}){
     auto n{file.integer<uint16_t>()};check(label+"damage size",values.size(),size_t(n));
     for(size_t j{};j<n;++j)check(label+"damage "+std::to_string(j),values.at(j),file.integer<int16_t>());
    }
    if(extended){
     check(label+"torso rate",body.torso().rate,get.template operator()<int16_t>(0x22a));
     check(label+"elevation rate",body.elevation().rate,get.template operator()<int16_t>(0x22c));
    }
    check(label+"steering",motion.steering_rate,get.template operator()<int16_t>(0x222));
    check(label+"gait",motion.gait_rate,get.template operator()<int16_t>(0x224));
    check(label+"throttle",motion.throttle,get.template operator()<int16_t>(0x226));
    check(label+"torso",body.torso().angle,get.template operator()<int16_t>(0x22e));
    check(label+"elevation",body.elevation().angle,get.template operator()<int16_t>(0x230));
    check(label+"power",actor.power().reserve,get.template operator()<int16_t>(0x228));
    check(label+"front shield",actor.power().shields.front,get.template operator()<int16_t>(0x1b8));
    check(label+"rear shield",actor.power().shields.rear,get.template operator()<int16_t>(0x1ba));
   }else if(kind==6){auto const &actor{flyers.at(f++).actor};position=actor.motion().position;angles=actor.angles();}
   else if(kind==5){auto const &actor{bases.at(b++).actor};position=actor.placement().translation;angles={get.template operator()<uint16_t>(6),get.template operator()<uint16_t>(8),get.template operator()<uint16_t>(10)};}
   else file.fail("actor kind");
   for(unsigned axis{};axis<3;++axis){
    check(label+"position "+std::to_string(axis),position[axis],get.template operator()<int32_t>(32+axis*4));
    if(kind!=5)check(label+"angle "+std::to_string(axis),angles[axis],get.template operator()<uint16_t>(6+axis*2));
   }
  }
  auto ng{file.integer<uint16_t>()};auto const &activity{session.activity_state()};
  check("group count",activity.all_groups().size(),size_t(ng));
  for(size_t g{};g<ng;++g){
   auto data{file.take(40)};auto const &group{activity.group(g)};
   binary_reader r{data.subspan(4)};check("group order",group.order,r.integer<int16_t>());check("group route",group.route_cursor,r.integer<int16_t>());
  }
  if(errors){std::cerr<<"Native HUD target "<<(session.target_selection()?int(*session.target_selection()):-1)<<'\n';std::cerr<<errors<<" differing fields\n";return 1;}
  ++checked;
  if(frame%100==0)std::cout<<frame<<" original states agree\n";
 }
 if(argc==3){
  if(terminal){
   auto compare_file=[&](char const *name,std::vector<uint8_t> const &actual){
    auto expected{file.take(file.integer<uint32_t>())};
    if(actual.size()!=expected.size())throw std::runtime_error{std::string(name)+" size: native "+std::to_string(actual.size())+", original "+std::to_string(expected.size())};
    for(size_t i{};i<actual.size();++i)if(actual[i]!=expected[i])
     throw std::runtime_error{std::string(name)+" byte "+std::to_string(i)+": native "+std::to_string(actual[i])+", original "+std::to_string(expected[i])};
   };
   compare_file("RESULTS.DAT",write_mission_results(view.results()));
   compare_file("MISSION.VAR",write_mission_counters(view.mission_variables()));
  }
  file.require_end();
  if(terminal&&next_packet!=expected_packets)file.fail("recorded input consumption");
  if(terminal&&expected_packets!=tape.frames.size())file.fail("source TAP packet count");
  if(terminal&&alert_count&&!exited)file.fail("terminal dialog did not leave mission");
 }
 std::cout<<modal_packets<<" packets consumed by native modal controls\n";
 std::cout<<"DEMO"<<demo<<": "<<checked<<" original states agree (including initial/EOF/exit when present)\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
