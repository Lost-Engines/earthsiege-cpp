#include "hmp.h"
#include <algorithm>

namespace earthsiege {
uint32_t read_hmp_delta(binary_reader &file) {
  uint32_t value{};
  for(unsigned int shift{};shift<35;shift+=7) {
    auto const byte{file.integer<uint8_t>()}; value|=static_cast<uint32_t>(byte&127)<<shift;
    if(byte&128)return value;
  }
  file.fail("unterminated HMP delta");
}
hmp_song read_hmp(bytes const data) {
  binary_reader f{data,"HMP music"};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="HMIMIDIP")f.fail("signature");
  hmp_song song;song.data.assign(data.begin(),data.end());f.seek(32);auto const branches{f.integer<uint32_t>()};
  f.seek(48);auto const count{f.integer<uint32_t>()};if(count>32)f.fail("track count");
  f.integer<uint32_t>();song.tick_rate=f.integer<uint32_t>();f.integer<uint32_t>();
  if(song.tick_rate==0)f.fail("zero timer rate");
  for(auto &priority:song.channel_priorities)priority=f.integer<uint32_t>();
  std::array<std::array<uint32_t,5>,32> routing;
  for(auto &track:routing)for(auto &id:track)id=f.integer<uint32_t>();
  f.seek(0x308);
  for(size_t i{};i<count;++i) {
    hmp_track track;track.offset=static_cast<uint32_t>(f.position());f.integer<uint32_t>();track.length=f.integer<uint32_t>();track.channel=f.integer<uint32_t>();track.devices=routing[i];
    if(track.length<12 || track.length>data.size()-track.offset)f.fail("track extent");
    song.tracks.push_back(std::move(track));f.seek(song.tracks.back().offset+song.tracks.back().length);
  }
  if(branches) {
    f.seek(branches);auto const counts{f.take(count)};
    for(size_t i{};i<count;++i)for(unsigned int j{};j<counts[i];++j) {
      hmp_branch branch;branch.event_offset=f.integer<uint32_t>();branch.id=f.integer<uint8_t>();branch.program=f.integer<uint8_t>();branch.loops=f.integer<uint8_t>();auto const length{f.integer<uint8_t>()};
      f.integer<uint32_t>();auto const restore{f.integer<uint32_t>()};f.integer<uint32_t>();f.integer<uint32_t>();
      if(branch.event_offset>=song.tracks[i].length-12 || restore>data.size() || length>data.size()-restore || (length&1))f.fail("branch extent");
      branch.controls.assign(data.begin()+restore,data.begin()+restore+length);song.tracks[i].branches.push_back(std::move(branch));
    }
  }
  return song;
}
} // namespace earthsiege
