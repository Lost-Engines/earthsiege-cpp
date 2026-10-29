#include "audio/audio_controls.h"
#include "resources/binary_reader.h"
#include <iostream>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const data{read_file(argv[1])};binary_reader f{data};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESAUCT01")f.fail("fixture version");
  std::array<servo_sound_state,2> servos;target_sound_state target;auto const cases{f.integer<uint32_t>()};size_t events{};
  for(uint32_t i{};i<cases;++i){
    auto const op{f.integer<uint8_t>()};auto const control{f.integer<int16_t>()},angle{f.integer<int16_t>()};auto const clock{f.integer<uint32_t>()};
    bool const locked{f.integer<uint8_t>()!=0},changed{f.integer<uint8_t>()!=0};auto const view{f.integer<uint8_t>()};
    bool const startup{f.integer<uint8_t>()!=0},servo{f.integer<uint8_t>()!=0},engine{f.integer<uint8_t>()!=0};
    std::optional<sound_action> action;uint16_t id{};
    if(op<2){action=advance_servo_sound(servos[op],control,angle,servo);id=33;}
    else if(op==2){if(auto const tone{advance_target_sound(target,locked,changed,clock)}){action=sound_action::play;id=*tone;}}
    else{action=engine_sound_gate(view==4,engine,startup);id=45;}
    auto fail{[&](char const *what){throw std::runtime_error{std::format("audio control {}: {}",i,what)};}};
    if(f.integer<uint8_t>()!=action.has_value())fail("event count");
    if(action){bool const stop{f.integer<uint8_t>()!=0};if(stop!=(*action==sound_action::stop) || id!=f.integer<uint16_t>())fail("sound action");++events;}
    for(auto const &s:servos)if(s.active!=(f.integer<uint8_t>()!=0) || s.previous!=f.integer<int16_t>())fail("servo latch");
    if(target.pulse!=(f.integer<uint8_t>()!=0) || target.previous_lock!=(f.integer<uint8_t>()!=0))fail("target latches");
  }
  f.require_end();std::cout<<cases<<" original servo/target/engine control states and "<<events<<" events match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
