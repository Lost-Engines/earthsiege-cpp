#include "resources/dfl_movie.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;auto data{read_file(argv[2])};binary_reader expected{data};expected.take(8);unsigned frames{};
 for(unsigned n{};n<2;++n){auto const name{expected.cstring()};auto const count{expected.integer<uint32_t>()};dfl_movie movie{read_file(std::filesystem::path{argv[1]}/"FLIC"/(name+".DFL"))};
  for(unsigned f{};f<count;++f){if(f&&!movie.advance())throw std::runtime_error{"early film end"};auto const pixels{movie.pixels()};std::vector<uint8_t> raster{pixels.begin(),pixels.end()};for(auto const &rgb:movie.colours().dac_colours)for(auto c:rgb)raster.push_back(static_cast<uint8_t>((c<<2)|(c>>4)));
   if(crc32(raster)!=expected.integer<uint32_t>())throw std::runtime_error{name+" differs on frame "+std::to_string(f)};
   auto samples{movie.take_samples()};bool start{f==(n?0U:14U)};if(samples.size()!=(start?1U:0U)||(start&&normalise_path(samples[0])!=(n?"victory.raw":"xpintrob.raw")))throw std::runtime_error{"movie audio start"};++frames;
  }
  if(movie.advance()||!movie.finished())throw std::runtime_error{"film termination"};
 }
 expected.require_end();std::cout<<frames<<" original expansion film frames, palettes and sample starts match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
