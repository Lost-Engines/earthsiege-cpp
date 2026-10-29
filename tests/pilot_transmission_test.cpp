#include "shell/pilot_transmission.h"
#include "resources/resource_store.h"
#include <cstdlib>
#include <iostream>
#include <format>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESPSTA01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};size_t total{};
  for(uint32_t trial{};trial<count;++trial){
    bool const dead{file.integer<uint32_t>()!=0};auto const message{file.integer<uint32_t>()},steps{file.integer<uint32_t>()};
    pilot_transmission transmission{static_cast<uint8_t>(message),read_portrait_sync(resources.read("simvol0",std::format("snc/PB_{:02}000.SNC",message))),0};random_state random{17};bool ready{},cancel{};
    for(uint32_t step{};step<steps;++step){
      auto const ticks{file.integer<uint32_t>()};bool const shown{file.integer<uint8_t>()!=0};auto const calls{file.integer<uint32_t>()};
      auto const result{transmission.advance(ticks,shown,dead,random)};ready|=result.ready;cancel|=result.cancel;
      auto const check{[&](int64_t expected,int64_t actual){if(expected!=actual){std::cerr<<trial<<'/'<<step<<" expected "<<expected<<" actual "<<actual<<'\n';file.fail("pilot transmission");}}};
      check(calls,result.calls.size());for(auto const &call:result.calls){check(file.integer<int32_t>(),call[0]);check(file.integer<int32_t>(),call[1]);}
      auto const &s{transmission.state()};check(file.integer<int32_t>(),s.phase);check(file.integer<int32_t>(),s.previous);check(file.integer<int32_t>(),s.deadline);
      check(file.integer<int32_t>(),s.permanent_static);check(file.integer<int32_t>(),s.death_frame);check(file.integer<int32_t>(),s.previous_death_frame);
      check(file.integer<int32_t>(),ready);check(file.integer<int32_t>(),cancel);check(file.integer<int32_t>(),transmission.mouth());++total;
    }
    for(auto v:random.snapshot().words)if(v!=file.integer<uint16_t>())file.fail("UI RNG word");
    if(random.snapshot().first!=file.integer<uint8_t>() || random.snapshot().second!=file.integer<uint8_t>())file.fail("UI RNG indices");
  }
  file.require_end();std::cout<<total<<" original pilot radio state transitions match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
