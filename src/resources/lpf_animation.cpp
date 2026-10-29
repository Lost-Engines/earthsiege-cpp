#include "lpf_animation.h"
#include <algorithm>
namespace earthsiege {
lpf_animation::lpf_animation(bytes data){
 binary_reader f{data,"LPF animation"};
 if(f.integer<uint32_t>()!=0x2046504c)f.fail("signature");
 f.seek(6);auto pages{f.integer<uint16_t>()};
 f.seek(16);if(f.integer<uint32_t>()!=0x4d494e41)f.fail("content signature");
 width_=f.integer<uint16_t>();height_=f.integer<uint16_t>();
 f.seek(64);auto total{f.integer<uint32_t>()};fps_=f.integer<uint16_t>();
 if(!width_||!height_||size_t{width_}*height_>0x1000000||total<2||total>65536||pages>256||!fps_)f.fail("header bounds");
 records.reserve(total-1);
 for(uint32_t frame{};frame<total-1;++frame){
  int page{-1};
  for(unsigned p{};p<pages;++p){
   f.seek(0x500+p*6);auto first{f.integer<uint16_t>()},count{f.integer<uint16_t>()};
   if(frame>=first&&frame<uint32_t{first}+count)page=static_cast<int>(p);
  }
  if(page<0)f.fail("missing page");
  auto const offset{size_t{0xb00}+(static_cast<size_t>(page)<<16)};
  f.seek(offset);auto first{f.integer<uint16_t>()},count{f.integer<uint16_t>()};
  if(count&0xc000||frame<first||frame>=uint32_t{first}+count)f.fail("unsupported split page");
  size_t start{offset+8+count*2},length{};
  f.seek(offset+8);
  for(unsigned i{};i<=frame-first;++i){length=f.integer<uint16_t>();if(i<frame-first)start+=length;}
  f.seek(start);auto block{f.take(length)};binary_reader record{block,"LPF record"};
  record.take(1);auto extra{record.integer<uint8_t>()};auto size{record.integer<uint16_t>()};
  if(extra)record.take(size+(size&1));
  auto payload{record.take(record.remaining())};records.emplace_back(payload.begin(),payload.end());
 }
}
void lpf_animation::decode(unsigned frame,std::span<uint8_t> target,bytes saved)const{
 binary_reader f{records.at(frame),"LPF delta"};
 if(target.size()!=size_t{width_}*height_||(!saved.empty()&&saved.size()!=target.size()))f.fail("image dimensions");
 size_t cursor{};
 auto space{[&](size_t count){if(count>target.size()-cursor)f.fail("span outside image");}};
 auto put{[&](uint8_t value){target[cursor]=value||saved.empty()?value:saved[cursor];++cursor;}};
 auto literal{[&](size_t count){space(count);for(auto value:f.take(count))put(value);}};
 auto fill{[&](size_t count){space(count);auto value{f.integer<uint8_t>()};while(count--)put(value);}};
 auto skip{[&](size_t count){space(count);cursor+=count;}};
 for(;;){
  auto op{f.integer<uint8_t>()};
  if(op>0&&op<128)literal(op);
  else if(!op)fill(f.integer<uint8_t>());
  else if(op!=128)skip(op&127);
  else{
   auto word{f.integer<uint16_t>()};
   if(!word)return;
   if(word<0x8000)skip(word);
   else if(word&0x4000)fill(word&0x3fff);
   else literal(word&0x3fff);
  }
 }
}
}
