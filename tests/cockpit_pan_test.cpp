#include "render/cockpit_pan.h"
#include "resources/binary_reader.h"
#include "shell/mission_view.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
  if(argc!=3)return 1;
  auto data{read_file(argv[1])};binary_reader f{data};
  auto magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESPANS01")f.fail("pan fixture version");
  auto cases{f.integer<uint32_t>()};unsigned total{};
  for(unsigned c{};c<cases;++c){
    auto action{f.integer<int32_t>()},view{f.integer<int32_t>()};
    auto x{f.integer<int32_t>()},y{f.integer<int32_t>()};
    std::array<cockpit_window,4> windows;
    for(auto &window:windows)for(auto &v:window.instrument_origin)v=f.integer<int32_t>();
    cockpit_surface surface;surface.restore(f.take(640*400));
    auto path{cockpit_pan_path(action,view,windows)};
    auto count{f.integer<uint32_t>()};
    if(path.size()!=count)f.fail("original pan frame count");
    framebuffer frame;
    for(auto const &position:path){
      auto expected_x{f.integer<int32_t>()},expected_y{f.integer<int32_t>()};
      if(position.x+x!=expected_x||position.y+y!=expected_y)f.fail("original scanout origin");
      surface.sample(frame,position.x+x,position.y+y);
      if(crc32(frame.pixels())!=f.integer<uint32_t>())f.fail("original pan pixels");
      ++total;
    }
  }
  f.require_end();
  resource_store resources{argv[2]};mission_view scene{resources,1};scene.draw();
  uint32_t clock{};unsigned live_paths{},live_frames{};
  for(auto const command:std::array<uint16_t,6>{65,59,67,68,68,67}){
    unsigned paths{};
    for(unsigned step{};step<6;++step){
      tape_frame input;input.delta=256;input.command=step==0?command:0;
      if(!scene.step_recorded(input,clock+=6))throw std::runtime_error{"unexpected mission exit"};
      scene.draw();auto frames{scene.take_pan_frames()};
      if(frames.empty())continue;
      if(crc32(frames.back().pixels())!=crc32(scene.image().pixels()))
        throw std::runtime_error{"pan does not finish on its incoming panel"};
      if(crc32(frames.front().pixels())==crc32(frames.back().pixels()))
        throw std::runtime_error{"pan does not show an intermediate view"};
      ++paths;live_frames+=static_cast<unsigned>(frames.size());
    }
    if(paths!=1)throw std::runtime_error{"live command did not produce exactly one pan"};
    live_paths+=paths;
  }
  std::cout<<live_paths<<" live panel transitions, "<<live_frames<<" queued frames; ";
  std::cout<<cases<<" original pan directions, "<<total<<" complete indexed frames match\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
