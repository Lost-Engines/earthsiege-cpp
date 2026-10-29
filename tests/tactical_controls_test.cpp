#include <cstdlib>
#include <iostream>
#include "shell/tactical_controller.h"
#include "resources/resource_store.h"
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return EXIT_FAILURE;auto const bytes{read_file(argv[1])};binary_reader f{bytes};
  auto const magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESTCTL01")f.fail("fixture version");
  rectangle const bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
  auto const player{f.integer<int32_t>()},range{f.integer<int32_t>()};auto const count{f.integer<uint32_t>()};
  std::vector<tactical_contact> contacts;
  for(size_t i{};i<count;++i){tactical_contact c;c.actor=i;c.position={f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};c.side=f.integer<uint8_t>();contacts.push_back(c);}
  tactical_controller controller;size_t row{};
  auto const compare{[&]{auto const &s{controller.state()};
    auto check{[&](auto value,auto expected,char const *label){if(value!=expected)f.fail("row "+std::to_string(row)+" "+label+": "+std::to_string(value)+" != "+std::to_string(expected));}};
    auto integer{[&](int value,char const *label){check(value,f.integer<int32_t>(),label);}};
    auto flag{[&](bool value,char const *label){check(value,f.integer<uint8_t>()!=0,label);}};
    auto vector{[&](vec3<int32_t> const &v,char const *label){integer(v.x,label);integer(v.y,label);integer(v.z,label);}};
    integer(s.command,"command");integer(s.previous,"previous");integer(s.wingmate,"wingmate");flag(s.requires_destination,"requires");flag(s.ready,"ready");flag(s.cursor,"cursor");flag(s.blink,"blink");
    integer(s.pan.x,"pan.x");integer(s.pan.y,"pan.y");integer(s.camera.z,"height");
    for(auto const &o:s.orders){vector(o.committed,"committed");vector(o.pending,"pending");integer(o.committed_actor?static_cast<int>(*o.committed_actor):-1,"committed actor");integer(o.pending_actor?static_cast<int>(*o.pending_actor):-1,"pending actor");flag(o.committed_active,"committed active");flag(o.pending_active,"pending active");integer(o.committed_kind,"committed kind");integer(o.pending_kind,"pending kind");}
    vector(s.packet.position,"packet position");integer(s.packet.target?static_cast<int>(*s.packet.target):-1,"packet actor");
  }};
  compare();auto const cases{f.integer<uint32_t>()};
  for(row=1;row<=cases;++row){auto const op{f.integer<uint8_t>()};auto const a{f.integer<int32_t>()},b{f.integer<int32_t>()};
    switch(op){
      case 0:controller.wingmate(a);break;
      case 1:controller.choose(a);break;
      case 2:{auto const [p,actor]{controller.pick(a,b,bounds,contacts)};controller.destination(p,actor,contacts,contacts.at(static_cast<size_t>(player)).position,range);break;}
      case 3:controller.commit();break;
      case 4:controller.packet(static_cast<size_t>(player));break;
      case 5:controller.cancel();break;
      case 6:controller.pan(-1,0);break;
      case 7:controller.pan(1,0);break;
      case 8:controller.pan(0,1);break;
      case 9:controller.pan(0,-1);break;
      case 10:controller.zoom(true);break;
      case 11:controller.zoom(false);break;
      default:f.fail("operation");
    }compare();
  }
  f.require_end();std::cout<<cases<<" original tactical control states match\n";return EXIT_SUCCESS;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
