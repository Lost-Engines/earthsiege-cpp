#include "resources/radio_catalogue.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};radio_catalogue catalogue{resources};
  auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESRDES01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::array<radio_speaker,3> speakers{{{0,1,true},{1,2,false},{2,4,false}}};
  auto const read{[&]{return file.integer<int32_t>();}};
  auto const actor{[](int n)->std::optional<size_t>{return n<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(n)};}};
  for(uint32_t i{};i<count;++i) {
    auto const speaker{read()},id{read()},seed{read()},expected_speaker{read()};bool const expected{file.integer<uint8_t>()!=0};
    radio_event event{static_cast<uint16_t>(id),actor(speaker),{12,34,56},2};random_state random{static_cast<int16_t>(seed)};
    auto const result{catalogue.resolve(event,speakers,random)};
    if(result.has_value()!=expected || event.speaker!=actor(expected_speaker))file.fail("resolution "+std::to_string(i));
    if(result) {
      radio_queue queue;queue.push(result->descriptor,result->text,event,150);auto const &message{*queue.front()};
      bool ok{true};ok&=message.event.id==read();ok&=message.event.speaker==actor(read());ok&=message.flags==read();ok&=message.priority==read();
      ok&=message.minimum==static_cast<uint32_t>(read());ok&=message.maximum==static_cast<uint32_t>(read());
      ok&=message.earliest==static_cast<uint32_t>(read());ok&=message.expiry==static_cast<uint32_t>(read());
      auto const text{file.take(file.integer<uint16_t>())};ok&=catalogue.at(result->text).text==std::string(text.begin(),text.end());
      if(!ok)file.fail("resolved message "+std::to_string(i));
    }
    for(auto word:random.snapshot().words)if(word!=file.integer<uint16_t>())file.fail("UI RNG words");
    if(random.snapshot().first!=file.integer<uint8_t>() || random.snapshot().second!=file.integer<uint8_t>())file.fail("UI RNG indices");
  }
  file.require_end();std::cout<<count<<" original pilot/system/commander radio resolutions match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
