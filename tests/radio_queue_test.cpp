#include "simulation/radio_queue.h"
#include "resources/binary_reader.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESRQUE01") file.fail("fixture version");
  auto const read{[&]{return file.integer<int32_t>();}};
  auto const reference{[&]()->std::optional<size_t>{auto const n{read()};return n ? std::optional<size_t>{static_cast<size_t>(n-1)} : std::nullopt;}};
  auto const event{[&]{radio_event e;e.id=static_cast<uint16_t>(read());e.speaker=reference();e.position={read(),read(),read()};e.subject=reference();return e;}};
  auto const trials{file.integer<uint32_t>()};size_t total{};
  for(uint32_t trial{};trial<trials;++trial) {
    radio_queue queue;auto const count{file.integer<uint32_t>()};
    for(uint32_t step{};step<count;++step) {
      auto const kind{read()},ticks{read()},index{read()};auto const payload{event()};
      radio_descriptor descriptor;
      descriptor.id=static_cast<uint8_t>(read());descriptor.flags=static_cast<uint8_t>(read());descriptor.priority=static_cast<uint8_t>(read());
      descriptor.minimum=static_cast<uint8_t>(read());descriptor.maximum=static_cast<uint8_t>(read());descriptor.earliest=static_cast<uint8_t>(read());descriptor.expiry=static_cast<uint8_t>(read());
      auto const text{read()};
      if(kind==0)queue.push(descriptor,static_cast<uint32_t>(text),payload,static_cast<uint32_t>(ticks));
      else if(kind==1)queue.expire(static_cast<uint32_t>(ticks));
      else queue.erase(static_cast<size_t>(index));
      auto const check{[&](bool match,char const *what){if(!match)file.fail(std::to_string(trial)+"/"+std::to_string(step)+": "+what);}};
      auto const &state{queue.state()};check(state.count==static_cast<size_t>(read()),"count");
      for(auto value:state.order)check(value==read(),"index");
      for(auto value:state.allocated)check(value==(read()!=0),"allocation");
      for(auto const &slot:state.slots) {
        auto const expected{event()};check(slot.event.id==expected.id && slot.event.speaker==expected.speaker && slot.event.position==expected.position && slot.event.subject==expected.subject,"event");
        check(slot.flags==read(),"flags");check(slot.text==static_cast<uint32_t>(read()),"text");check(slot.priority==read(),"priority");
        check(slot.minimum==static_cast<uint32_t>(read()),"minimum");check(slot.maximum==static_cast<uint32_t>(read()),"maximum");
        check(slot.earliest==static_cast<uint32_t>(read()),"earliest");check(slot.expiry==static_cast<uint32_t>(read()),"expiry");
      }
      ++total;
    }
  }
  file.require_end();std::cout<<total<<" original radio queue operations match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
