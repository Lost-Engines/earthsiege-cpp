#include "simulation/instrument_failure.h"
#include "resources/binary_reader.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESIFAL01") file.fail("fixture version");
  for(int i{};i<=256;++i) if(instrument_damage_band(i)!=file.integer<uint8_t>()) file.fail("damage band");
  auto const traces{file.integer<uint32_t>()};size_t total{};
  for(uint32_t t{};t<traces;++t) {
    auto const seed{file.integer<int16_t>()};auto const severity{file.integer<uint16_t>()};auto const count{file.integer<uint32_t>()};
    random_state random{seed};instrument_failure_state state;schedule_instrument_failure(state,0,random);
    for(uint32_t step{};step<count;++step) {
      auto const ticks{file.integer<uint32_t>()};bool const sound{advance_instrument_failure(state,severity,ticks,random)};
      if(state.duration!=file.integer<uint16_t>() || state.window!=file.integer<uint32_t>() || state.next_flip!=file.integer<uint32_t>() || state.failed!=(file.integer<uint8_t>()!=0) || state.repaint!=file.integer<uint8_t>() || sound!=(file.integer<uint8_t>()!=0)) file.fail("failure timing at trace "+std::to_string(t)+" step "+std::to_string(step));
      ++total;
    }
    for(auto value:random.snapshot().words)if(value!=file.integer<uint16_t>())file.fail("UI RNG word");
    if(random.snapshot().first!=file.integer<uint8_t>() || random.snapshot().second!=file.integer<uint8_t>())file.fail("UI RNG indices");
  }
  file.require_end();std::cout<<total<<" original instrument failure updates and 257 damage bands match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
