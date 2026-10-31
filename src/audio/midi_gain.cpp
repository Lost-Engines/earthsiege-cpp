#include "midi_gain.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
midi_gain::midi_gain(std::span<uint32_t const> installed):devices{installed.begin(),installed.end()},volumes_(installed.size()){for(auto &v:volumes_)v.fill(127);}
auto midi_gain::apply(midi_packet packet,uint8_t song_volume,bool muted)->midi_packet {
  if(packet.wire.size()>=3 && (packet.wire[0]&0xf0)==0xb0 && packet.wire[1]==7){
    auto const found{std::ranges::find(devices,packet.device)};if(found==devices.end())throw std::runtime_error{"MIDI gain device"};
    auto const device{static_cast<size_t>(found-devices.begin())};auto const value{packet.wire[2]};volumes_[device][packet.wire[0]&15]=value;
    packet.wire[2]=muted?0:static_cast<uint8_t>(((value*song_volume)>>7)*master_>>7);
  }return packet;
}
auto midi_gain::set_master(uint8_t value)->std::vector<midi_packet> {
  master_=value;std::vector<midi_packet> result;
  for(size_t d{};d<devices.size();++d)for(uint8_t ch{};ch<16;++ch)if(devices[d]!=0xa004 || ((0x02feu>>ch)&1u))result.push_back({devices[d],{static_cast<uint8_t>(0xb0|ch),7,static_cast<uint8_t>(volumes_[d][ch]*value>>7)},{}});
  return result;
}
auto midi_gain::set_song(uint8_t value,std::span<std::pair<size_t,uint8_t> const> tracks)const->std::vector<midi_packet> {
  std::vector<midi_packet> result;for(auto const &[d,ch]:tracks)result.push_back({devices.at(d),{static_cast<uint8_t>(0xb0|ch),7,static_cast<uint8_t>(volumes_.at(d).at(ch)*value>>7)},{}});return result;
}
}
