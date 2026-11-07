#include "shell/demo_playback.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=4)throw std::runtime_error{"Usage: expansion_replay_test DATA FIXTURE SAVE_DIRECTORY"};
 auto const bytes{read_file(argv[2])};binary_reader expected{bytes};auto magic{expected.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESPATH02")expected.fail("demo path fixture version");
 auto const demo{expected.integer<uint32_t>()},frames{expected.integer<uint32_t>()},packets{expected.integer<uint32_t>()};
 auto const prefs{expected.take(expected.integer<uint16_t>())};
 resource_store resources{argv[1]};preferences settings{prefs,true,resources.edition()};
 demo_playback replay{resources,demo,settings,std::filesystem::path{argv[3]}/("demo"+std::to_string(demo))};
 auto const present{[&](unsigned,framebuffer const &){replay.view().take_audio();replay.view().take_pan_frames();return true;}};
 unsigned startup{};
 while(replay.view().starting()&&!replay.done()){
  if(++startup>20000)expected.fail("demo startup did not finish");
  replay.step(present);
 }
 for(uint32_t frame{};frame<frames;++frame){
  if(replay.done())expected.fail("demo ended before original frame "+std::to_string(frame));
  auto check{[&](auto actual,auto original,char const *field){
   if(actual!=original)expected.fail("demo "+std::to_string(demo)+" frame "+std::to_string(frame)+" "+field+": native "+std::to_string(actual)+", original "+std::to_string(original));
  }};
  auto const &random{replay.view().simulation().random_snapshot()};
  for(auto value:random.words)check(value,expected.integer<uint16_t>(),"gameplay random word");
  check(random.first,expected.integer<uint8_t>(),"gameplay random first tap");
  check(random.second,expected.integer<uint8_t>(),"gameplay random second tap");
  replay.step(present);
  check(replay.frames_played(),frame+1,"simulation count");
  check(replay.packets_read(),expected.integer<uint32_t>(),"input packets");
  auto const &motion{replay.view().simulation().player().movement()};
  for(auto value:{motion.position.x,motion.position.y,motion.position.z})check(value,expected.integer<int32_t>(),"position");
  for(auto value:{motion.angles.x,motion.angles.y,motion.angles.z})check(value,expected.integer<uint16_t>(),"angle");
  check(motion.throttle,expected.integer<int16_t>(),"throttle");
  check(motion.gait_rate,expected.integer<int16_t>(),"gait");
  check(motion.steering_rate,expected.integer<int16_t>(),"steering");
  check(motion.playback_rate,expected.integer<int16_t>(),"animation rate");
 }
 expected.require_end();
 if(!replay.done()||replay.packets_read()!=packets)expected.fail("demo did not end at the original packet");
 std::cout<<"Expansion demo "<<demo<<": "<<frames<<" player states and gameplay RNG agree exactly with DOSBox; "<<packets<<" packets, "<<replay.modal_packets_read()<<" modal packets\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
