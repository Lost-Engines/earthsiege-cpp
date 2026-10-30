#include <iostream>
#include <string>
#include "shell/mission_view.h"
#include "resources/resource_store.h"
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc<3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader file{raw};
 auto magic{file.take(8)};if(std::string(magic.begin(),magic.end())!="ESRPLY03")file.fail("format");
 auto demo{file.integer<uint32_t>()},count{file.integer<uint32_t>()};
 auto tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
 mission_view view{resources,tape};
 for(uint32_t frame{};frame<count;++frame){
  auto packet{file.integer<int32_t>()};auto clock{file.integer<uint32_t>()};auto ratio{file.integer<uint16_t>()};auto actors{file.integer<uint16_t>()};
  if(argc>3&&frame>std::stoul(argv[3]))break;
  if(packet!=-1){
   tape_frame record;
   if(packet>=0)record=tape.frames.at(static_cast<size_t>(packet));
   else record.delta=ratio; // EOF clears the packet but retains the outer ratio.
   if(record.delta!=ratio)file.fail("recorded outer ratio");
   view.step_recorded(record,clock);view.draw();
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
   bytes data{storage};auto get=[&]<class T>(size_t at){binary_reader r{data.subspan(at)};return r.integer<T>();};
   auto kind{get.template operator()<uint16_t>(0)};
   auto expected_target{file.integer<int32_t>()};auto expected_controller{file.integer<int16_t>()};
   auto const &relationship{session.activity_state().targets()[a]};
   auto const &controller{session.activity_state().decisions()[a].controller};
   check("actor "+std::to_string(a)+" target",relationship.selected?int32_t(*relationship.selected):-1,expected_target);
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
  if(frame%100==0)std::cout<<frame<<" original states agree\n";
 }
 if(argc==3)file.require_end();
 std::cout<<"DEMO"<<demo<<": "<<count<<" original states agree (including initial/EOF when present)\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
