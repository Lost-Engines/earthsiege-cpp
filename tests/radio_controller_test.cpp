#include "shell/radio_controller.h"
#include "resources/binary_reader.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESRSTA01")file.fail("fixture version");
  auto const read{[&]{return file.integer<int32_t>();}};auto const trials{file.integer<uint32_t>()};size_t total{};
  for(uint32_t trial{};trial<trials;++trial){
    radio_controller radio;auto const count{file.integer<uint32_t>()};
    for(uint32_t step{};step<count;++step){
      auto const ticks{static_cast<uint32_t>(read())};bool const enqueue{read()!=0},ready{read()!=0},cancel{read()!=0},enabled{read()!=0},can_show{read()!=0},can_finish{read()!=0};auto const text{static_cast<uint32_t>(read())};
      radio_descriptor d;d.id=static_cast<uint8_t>(read());d.flags=static_cast<uint8_t>(read());d.priority=static_cast<uint8_t>(read());d.minimum=static_cast<uint8_t>(read());d.maximum=static_cast<uint8_t>(read());d.earliest=static_cast<uint8_t>(read());d.expiry=static_cast<uint8_t>(read());
      if(enqueue)radio.queue().push(d,text,{},ticks);
      if(ready)radio.ready();
      if(cancel)radio.cancel();
      std::vector<std::pair<int,int>> calls;
      radio.advance(ticks,enabled,{
        .start{[&](radio_message const &m){calls.emplace_back(0,static_cast<int>(m.text));}},
        .show{[&](radio_message const &){calls.emplace_back(1,can_show);return can_show;}},
        .finish{[&](radio_message const &){calls.emplace_back(2,can_finish);return can_finish;}}
      });
      auto const check{[&](int64_t expected,int64_t actual){if(expected!=actual){std::cerr<<trial<<'/'<<step<<" expected "<<expected<<" actual "<<actual<<'\n';file.fail("radio state");}}};
      check(file.integer<uint32_t>(),calls.size());for(auto const &[kind,value]:calls){check(read(),kind);check(read(),value);}
      auto const &s{radio.state()};auto const &q{radio.queue().state()};
      check(read(),q.count);check(read(),s.active?static_cast<int>(*s.active):-1);check(read(),s.starting);check(read(),s.ready);check(read(),s.cancelling);check(read(),s.visible);
      for(auto v:q.order)check(read(),v);
      for(auto v:q.allocated)check(read(),v);
      for(auto const &m:q.slots){check(read(),m.text);check(read(),m.minimum);check(read(),m.maximum);check(read(),m.earliest);check(read(),m.expiry);}
      ++total;
    }
  }
  file.require_end();
  auto const cancellations{read_file(argv[2])};binary_reader cases{cancellations};auto const signature{cases.take(8)};
  if(std::string(signature.begin(),signature.end())!="ESRCAN01")cases.fail("cancellation fixture");
  auto const case_count{cases.integer<uint32_t>()};
  for(uint32_t i{};i<case_count;++i){
    auto phase{cases.integer<int32_t>()},id{cases.integer<int32_t>()},speaker{cases.integer<int32_t>()};auto result{cases.integer<int32_t>()};
    radio_controller radio;uint32_t text{};
    for(auto [message,actor]:std::array<std::pair<uint8_t,size_t>,3>{{{34,1},{30,1},{34,2}}})
      radio.queue().push({message,0,0,1,3,0,10},++text,{message,actor,{},{}},0);
    radio_presentation_callbacks callbacks{[](auto const &){},[](auto const &){return true;},[](auto const &){return true;}};
    if(phase)radio.advance(1,true,callbacks);
    if(phase==2){radio.ready();radio.advance(2,true,callbacks);}
    auto check=[&](int64_t expected,int64_t actual){if(expected!=actual)cases.fail("cancellation case "+std::to_string(i));};
    check(result,radio.cancel_message(static_cast<uint16_t>(id),static_cast<size_t>(speaker)));
    auto const &s{radio.state()};auto const &q{radio.queue().state()};
    check(cases.integer<int32_t>(),q.count);check(cases.integer<int32_t>(),s.active?int(*s.active):-1);
    for(bool flag:{s.starting,s.ready,s.cancelling,s.visible})check(cases.integer<int32_t>(),flag);
    for(auto v:q.order)check(cases.integer<int32_t>(),v);
    for(auto v:q.allocated)check(cases.integer<int32_t>(),v);
  }
  cases.require_end();std::cout<<case_count<<" original message cancellation cases match\n";
  std::cout<<total<<" original active-radio transitions match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
