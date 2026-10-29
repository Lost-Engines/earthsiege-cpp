#include "simulation/footsteps.h"
#include <iostream>
#include <format>

int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const data{read_file(argv[1])};binary_reader f{data};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESFOOT01")f.fail("fixture version");
  auto const count{f.integer<uint32_t>()};size_t emitted{};
  movement_profile profile{.movement_sequence{0},.fast_sequence{1},.forward_stop_sequence{2},.reverse_stop_sequence{3},.turn_sequence{5},.disabled_sequence{4}};
  for(uint32_t trial{};trial<count;++trial){
    animation_channel_state channel;channel.sequence=f.integer<int16_t>();channel.requested_sequence=f.integer<int16_t>();auto const rate{f.integer<int16_t>()};bool const player{f.integer<uint8_t>()!=0};
    f.integer<int32_t>();f.integer<int32_t>();auto const z{f.integer<int32_t>()};f.integer<uint16_t>();
    std::array<int16_t,10> trigger{},rearm{};for(size_t i{};i<4;++i)trigger[i]=f.integer<int16_t>();for(size_t i{};i<4;++i)rearm[i]=f.integer<int16_t>();
    footstep_state state;for(auto &l:state.latches)l=f.integer<uint8_t>();
    std::array<footstep_attachment,5> parts;
    for(auto &p:parts){p.type=f.integer<int8_t>();p.active=f.integer<uint8_t>()!=0;f.integer<int32_t>();p.local_y=f.integer<int32_t>();p.world_z=f.integer<int32_t>()+z;}
    auto const events{advance_footsteps(state,profile,trigger,rearm,channel,rate,player,parts)};
    auto fail{[&](char const *what){throw std::runtime_error{std::format("footstep case {}: {}",trial,what)};}};
    if(events.size()!=f.integer<uint32_t>())fail("sound count");
    for(auto const id:events)if(id!=f.integer<uint16_t>())fail("sound id");
    for(auto const l:state.latches)if(l!=f.integer<uint8_t>())fail("retained latch");
    emitted+=events.size();
  }
  f.require_end();std::cout<<count<<" original attachment sound/latch states and "<<emitted<<" sounds match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
