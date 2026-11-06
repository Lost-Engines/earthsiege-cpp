#include "shell/career_transfer.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
bytes block(binary_reader &r){return r.take(r.integer<uint32_t>());}
random_state rng(binary_reader &r){random_state::state s;for(auto &w:s.words)w=r.integer<uint16_t>();s.first=r.integer<uint8_t>();s.second=r.integer<uint8_t>();return random_state{s};}
void equal(bytes a,bytes b,std::string const &label){if(std::ranges::equal(a,b))return;auto m{std::ranges::mismatch(a,b)};throw std::runtime_error{label+" offset "+std::to_string(m.in1-a.begin())+" values "+(m.in1==a.end()?"EOF":std::to_string(*m.in1))+"/"+(m.in2==b.end()?"EOF":std::to_string(*m.in2))};}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};f.take(8);auto count{f.integer<uint16_t>()};
 for(unsigned i{};i<count;++i){f.integer<int16_t>();auto keep{f.integer<int16_t>()};f.integer<int16_t>();auto before{block(f)},hangars{block(f)};auto random{rng(f)};auto expected{block(f)},expected_hangars{block(f)};auto expected_random{rng(f)};
  campaign_start source;source.career=read_career(before,game_edition::expansion);binary_reader h{hangars};for(auto &roster:source.hangars)roster=read_hangar(h,game_edition::expansion);h.require_end();
  auto out{transfer_career(resources,source,keep!=0,random)};equal(write_career(out.career),expected,"transfer "+std::to_string(i));binary_writer hw;for(auto const &roster:out.hangars)hw.append(write_hangar(roster));equal(std::move(hw).finish(),expected_hangars,"transferred hangars");
  auto a{random.snapshot()},b{expected_random.snapshot()};if(a.words!=b.words||a.first!=b.first||a.second!=b.second)throw std::runtime_error{"transfer random state"};
 }
 f.require_end();std::cout<<count<<" original career transfers match, including all history, hangars and RNG\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
