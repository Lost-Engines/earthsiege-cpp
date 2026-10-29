#include "audio/combat_music.h"
#include "resources/binary_reader.h"
#include <iostream>
#include <format>
#include <utility>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader r{raw};auto const magic{r.take(8)};if(std::string(magic.begin(),magic.end())!="ESMSEL01")r.fail("fixture version");auto const count{r.integer<uint32_t>()};
  auto state=[&]{combat_music_state s;s.track=r.integer<int16_t>();s.battle=r.integer<uint8_t>()!=0;s.deadline=r.integer<uint32_t>();s.fading=r.integer<uint8_t>()!=0;return s;};
  auto rng=[&]{random_state::state s;for(auto &w:s.words)w=r.integer<uint16_t>();s.first=r.integer<uint8_t>();s.second=r.integer<uint8_t>();return s;};
  for(uint32_t i{};i<count;++i){auto s{state()};music_playback p;for(auto &v:p.volume)v=r.integer<uint8_t>();for(auto &v:p.playing)v=r.integer<uint8_t>()!=0;random_state random{rng()};auto const clock{r.integer<uint32_t>()};bool const enabled{r.integer<uint8_t>()!=0},driver{r.integer<uint8_t>()!=0};
    auto contacts=[&]{std::vector<music_contact> out(r.integer<uint8_t>());for(auto &a:out){a.distance=r.integer<int32_t>();a.side=r.integer<uint8_t>();a.disabled=r.integer<uint8_t>()!=0;a.dead=r.integer<uint8_t>()!=0;a.waiting=r.integer<uint8_t>()!=0;a.visible=r.integer<uint8_t>()!=0;}return out;};auto const actors{contacts()},flyers{contacts()};
    auto const events{advance_combat_music(s,p,actors,flyers,clock,enabled,driver,random)};auto const expected{state()};
    if(s.track!=expected.track||s.battle!=expected.battle||s.deadline!=expected.deadline||s.fading!=expected.fading)throw std::runtime_error{std::format("music decision {} state",i)};
    for(auto const v:p.volume)if(v!=r.integer<uint8_t>())throw std::runtime_error{"fade volume"};
    for(auto const v:p.playing)if(v!=(r.integer<uint8_t>()!=0))throw std::runtime_error{"playback state"};
    auto const next{rng()};auto const &actual{random.snapshot()};
    if(actual.words!=next.words||actual.first!=next.first||actual.second!=next.second)throw std::runtime_error{"music RNG"};
    auto const n{r.integer<uint8_t>()};unsigned int read{};for(auto const &e:events)if(e.action!=music_action::volume){if(read>=n||std::to_underlying(e.action)!=r.integer<uint8_t>()||e.track!=r.integer<uint8_t>())throw std::runtime_error{"music request"};++read;}if(read!=n)throw std::runtime_error{"missing music request"};
  }r.require_end();std::cout<<count<<" original music decisions, fades, playback requests and RNG states match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
