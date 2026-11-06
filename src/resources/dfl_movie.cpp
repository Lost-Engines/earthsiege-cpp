#include "dfl_movie.h"
#include <algorithm>
#include <utility>
#include <stdexcept>
namespace earthsiege {
dfl_movie::dfl_movie(bytes data):source{data.begin(),data.end()}{
 binary_reader f{data,"DFL header"};
 if(f.integer<uint32_t>()!=data.size()||f.integer<uint16_t>()!=0xaf12)f.fail("unsupported film header");
 f.integer<uint16_t>();if(f.integer<uint16_t>()!=320||f.integer<uint16_t>()!=200)f.fail("unsupported film dimensions");
 if(data.size()<128)f.fail("short film header");
 advance();
}
void dfl_movie::delta(bytes data){
 binary_reader f{data,"DFL delta"};auto const count{f.integer<uint32_t>()&65535};int destination{};
 for(unsigned i{};i<count;++i){auto const word{f.integer<uint16_t>()};auto const op{word&7};
  if(op==0){destination+=word>>3;continue;}
  int amount{},offset{};
  if(op==1){amount=word>>3;if(destination<0||destination+amount>static_cast<int>(raster.size()))f.fail("literal outside frame");auto v{f.take(static_cast<size_t>(amount))};std::ranges::copy(v,raster.begin()+destination);destination+=amount;continue;}
  if(op==2||op==4||op==5){offset=std::bit_cast<int8_t>(static_cast<uint8_t>(word))>>3;amount=word>>8;if(op==4)offset-=320;if(op==5)offset+=320;}
  else if(op==3){offset=std::bit_cast<int8_t>(static_cast<uint8_t>(word>>8));amount=(word&255)>>3;}
  else if(op==6){offset=std::bit_cast<int16_t>(word)>>3;amount=f.integer<uint16_t>();}
  else {amount=word>>3;offset=f.integer<int16_t>();}
  int const from{destination+offset};
  if(std::min(from,destination)<0||std::max(from+amount,destination+amount)>static_cast<int>(raster.size()))f.fail("copy outside frame");
  // REP MOVSW reads each complete word before writing it. A bytewise copy
  // produces a different result for overlapping copies with offset -1.
  int j{};for(;j+1<amount;j+=2){auto a{raster[static_cast<size_t>(from+j)]},b{raster[static_cast<size_t>(from+j+1)]};raster[static_cast<size_t>(destination+j)]=a;raster[static_cast<size_t>(destination+j+1)]=b;}
  if(j<amount)raster[static_cast<size_t>(destination+j)]=raster[static_cast<size_t>(from+j)];
  destination+=amount;
 }
 if(f.remaining()>1)f.fail("unparsed delta bytes");
}
void dfl_movie::palette_packets(bytes data){
 binary_reader f{data,"DFL palette"};auto const count{f.integer<uint16_t>()};unsigned index{};
 for(unsigned i{};i<count;++i){index+=f.integer<uint8_t>();unsigned amount{f.integer<uint8_t>()};if(!amount)amount=256;if(index+amount>256)f.fail("palette packet outside bank");auto values{f.take(amount*3)};std::ranges::copy(values,base_palette.begin()+index*3);std::ranges::copy(values,display_palette.begin()+index*3);index+=amount;}
 if(f.remaining()>1)f.fail("unparsed palette bytes");
}
void dfl_movie::fade(int amount){for(size_t i{};i<768;++i)display_palette[i]=static_cast<uint8_t>(int(base_palette[i])+(fade_colour[i%3]-base_palette[i])*amount/fade_length);}
void dfl_movie::fade_step(){--fade_remaining;fade(fade_direction==7?fade_length-fade_remaining:fade_remaining);}
void dfl_movie::update_palette(){for(size_t i{};i<256;++i){for(size_t c{};c<3;++c)colours_.dac_colours[i][c]=display_palette[i*3+c]>>2;colours_.colours[i]=display_colour(colours_.dac_colours[i]);}}
bool dfl_movie::advance(){
 if(position==source.size()){done=true;return false;}
 if(fade_remaining)fade_step();
 binary_reader f{bytes{source}.subspan(position),"DFL frame"};auto const length{f.integer<uint32_t>()};f.integer<uint16_t>();auto const count{f.integer<uint16_t>()};f.take(8);
 if(length<16||length>source.size()-position)f.fail("invalid frame extent");
 for(unsigned i{};i<count;++i){auto const size{f.integer<uint32_t>()};auto const kind{f.integer<uint16_t>()};if(size<6||size>length||f.position()+size-6>length)f.fail("invalid chunk extent");auto const body{f.take(size-6)};
  if(kind==20)delta(body);
  else if(kind==4)palette_packets(body);
  else if(kind==13)raster.fill(0);
  else if(kind==22){binary_reader p{body};auto direction{p.integer<int32_t>()},mode{p.integer<int32_t>()};fade_length=std::max(1,p.integer<int32_t>());auto colour{p.integer<uint32_t>()};fade_colour={static_cast<int>((colour&0xf00)>>4),static_cast<int>(colour&0xf0),static_cast<int>((colour&15)<<4)};fade_direction=direction;
   if(mode==9){fade_remaining=fade_length;fade_step();++fade_remaining;}else if(mode==8){fade(direction==6?0:fade_length);fade_remaining=0;}else p.fail("unknown fade mode");
  }else if(kind==21){binary_reader p{body};auto action{p.integer<int32_t>()};p.integer<int32_t>();auto name{p.cstring()};if(action==1)samples.push_back(std::move(name));}
  else if(kind!=18&&kind!=23&&kind!=24)f.fail("unsupported film chunk");
 }
 if(f.position()!=length)f.fail("unparsed frame bytes");
 position+=length;update_palette();return true;
}
std::vector<std::string> dfl_movie::take_samples(){return std::exchange(samples,{});}
}
