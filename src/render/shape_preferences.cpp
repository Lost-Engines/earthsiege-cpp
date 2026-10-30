#include "shape_preferences.h"
#include <array>
namespace earthsiege {
shape_preferences shape_detail(uint8_t value,bool reduced_memory){
 size_t band{};while(band<4&&value>50*(band+1))++band;
 if(reduced_memory){
  constexpr std::array<int16_t,5> scales{2000,3000,4000,4000,4000};
  constexpr std::array<unsigned,5> roots{2,1,0,0,0},ordinary{2,1,1,1,1},weapon{1,1,0,0,0};
  return {{8,scales[band],roots[band]},ordinary[band],weapon[band],ordinary[band]};
 }
 constexpr std::array<unsigned,5> nested{2,2,1,1,0};
 return {{8,2000,static_cast<unsigned>(4-band)},nested[band],nested[band],nested[band]};
}
}
