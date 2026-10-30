#include "simulation/tape_input.h"
#include "resources/input_options.h"
#include <filesystem>
#include <iostream>
using namespace earthsiege;
void check(binary_reader &f,tape_frame const &in,device_input_sample const &out,device_routing_options const &o){
 if(f.integer<uint16_t>()!=in.command)f.fail("recorded command");
 for(unsigned i{};i<11;++i)if(f.integer<uint8_t>())f.fail("playback direct pointer fields must be zero");
 for(auto v:out.axes)if(f.integer<int16_t>()!=v)f.fail("post-read axes");
 for(auto v:out.buttons)if(f.integer<uint8_t>()!=v)f.fail("recorded buttons");
 for(auto v:out.hat)if(f.integer<uint8_t>()!=v)f.fail("recorded hat");
 if(f.integer<uint16_t>()!=in.delta)f.fail("recorded ratio");
 if(f.integer<uint8_t>()!=(out.aiming_camera?17:13))f.fail("camera alias");
 if(f.integer<uint8_t>()!=o.mode)f.fail("control mode");
}
void magic(binary_reader &f,char const *expected){auto s{f.take(8)};if(std::string(s.begin(),s.end())!=expected)f.fail("fixture version");}
int main(int argc,char **argv)try{
 if(argc!=4)return 1;
 auto data{read_file(argv[2])};binary_reader f{data};magic(f,"ESTPPK01");unsigned total{};
 auto const tapes{f.integer<uint32_t>()};
 for(unsigned t{};t<tapes;++t){
  auto const demo{f.integer<uint8_t>()};auto count{f.integer<uint32_t>()};
  auto tape{read_input_tape(read_file(std::filesystem::path{argv[1]}/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
  if(count!=tape.frames.size())f.fail("frame count");
  device_routing_options options;options.mode=tape.starting_files[std::to_underlying(tape_file::preferences)].at(8);
  for(auto const &frame:tape.frames){auto const output{route_tape_frame(options,frame)};check(f,frame,output,options);++total;}
 }
 f.require_end();
 auto cases{read_file(argv[3])};binary_reader c{cases};magic(c,"ESTPCR01");auto count{c.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  device_routing_options o;o.mode=c.integer<uint8_t>();o.wcs=c.integer<uint8_t>();o.external=c.integer<uint8_t>();o.missile=c.integer<uint8_t>();o.switch_held=c.integer<uint8_t>();o.backturn_reverse=c.integer<uint8_t>();
  bool suppress{c.integer<uint8_t>()!=0};auto lever{c.integer<int16_t>()};
  tape_frame in;in.command=c.integer<uint16_t>();in.primary_axes={c.integer<int16_t>(),c.integer<int16_t>()};in.secondary_axes={c.integer<int16_t>(),c.integer<int16_t>()};in.delta=c.integer<uint16_t>();in.buttons=c.integer<uint16_t>();
  auto out{route_tape_frame(o,in,suppress,lever)};check(c,in,out,o);
  if(c.integer<uint8_t>()!=o.switch_held)c.fail("button-2 switch latch");
 }
 c.require_end();std::cout<<total<<" original tape packets and "<<count<<" control contexts agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
