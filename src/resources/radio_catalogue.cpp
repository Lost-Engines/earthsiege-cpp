#include "radio_catalogue.h"
#include "resources/interface_strings.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
radio_catalogue::radio_catalogue(resource_store const &resources) {
  constexpr std::array names{"SYSTEM","COMMANDR","PILOT1","PILOT2","PILOT4"};
  for(size_t bank{};bank<banks.size();++bank) {
    auto const groups{read_interface_strings(resources.read("simvol0",std::string{"str/"}+names[bank]+".STR"))};
    for(auto const &group:groups) for(auto const &item:group) {
      if(item.attributes.size()!=7)throw std::runtime_error{"radio descriptor size"};
      auto const &d{item.attributes};
      banks[bank].push_back({item.text,{d[0],d[1],d[2],d[3],d[4],d[5],d[6]}});
    }
  }
}
std::optional<radio_resolution> radio_catalogue::resolve(radio_event &event, std::span<radio_speaker const> speakers, random_state &random) const {
  size_t bank{};
  if(event.speaker) {
    if(speakers.empty())return {};
    auto speaker{std::ranges::find(speakers,*event.speaker,&radio_speaker::actor)};
    if(speaker==speakers.end()){speaker=speakers.begin();event.speaker=speaker->actor;}
    if(static_cast<uint8_t>(event.id)>=50 && speaker->leader) {
      // Original loader uses bd0ac + id*9, whereas the resolver uses
      // bceea + id*9. No commander descriptors reach the intended table.
      // Keep the assets, but do not reproduce the loader's out-of-bounds writes.
      return {};
    }
    else switch(speaker->voice) {
      case 1:bank=2;break;case 2:bank=3;break;case 4:bank=4;break;
      default:throw std::runtime_error{"unsupported original pilot voice bank"};
    }
  }
  auto const &records{banks[bank]};
  auto const first{std::ranges::find_if(records,[&](auto const &item){return item.descriptor.id==static_cast<uint8_t>(event.id);})};
  if(first==records.end())return {};
  auto const count{std::ranges::count_if(records,[&](auto const &item){return item.descriptor.id==static_cast<uint8_t>(event.id);})};
  size_t index{static_cast<size_t>(first-records.begin())};
  // Original 0x2f31b repeatedly asks for the entry after the first record.
  // Supplied banks have at most two variants; preserve the call's nonzero case.
  if(count>1 && static_cast<uint16_t>(random.next())%static_cast<size_t>(count))++index;
  return radio_resolution{static_cast<uint32_t>((bank<<16)|index),records.at(index).descriptor};
}
radio_text const &radio_catalogue::at(uint32_t const text) const {return banks.at(text>>16).at(text&65535);}
} // namespace earthsiege
