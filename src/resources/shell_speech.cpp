#include "shell_speech.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
shell_speech_group read_shell_speech_group(bytes source){
 binary_reader f{source,"shell speech list"};
 auto count{f.integer<uint8_t>()};shell_speech_group out{f.cstring(),{}};
 for(unsigned i{};i<count;++i){
  if(f.integer<uint8_t>()!=2)f.fail("speech record must contain SNC and PCM names");
  out.clips.push_back({f.cstring(),f.cstring()});
 }
 f.require_end();return out;
}
std::vector<shell_mouth_frame> read_shell_mouth_timeline(bytes source){
 binary_reader f{source,"GO shell mouth timeline"};auto count{f.integer<uint16_t>()};
 if(count!=f.remaining()||count%4)f.fail("shell SNC byte count");
 std::vector<shell_mouth_frame> out;uint16_t deadline{};
 while(f.remaining()){
  auto const page{f.integer<uint16_t>()};deadline=static_cast<uint16_t>(deadline+f.integer<uint16_t>());
  out.push_back({page,deadline});
 }
 return out;
}
shell_mouth::shell_mouth(std::vector<shell_mouth_frame> value,uint16_t pages)
 :frames{std::move(value)},program_pages{pages}{
 if(frames.empty())throw std::runtime_error{"empty shell SNC"};
}
void shell_mouth::stop(){
 page_=program_pages>30?30:1;active_=pending_=false;
}
void shell_mouth::update(uint16_t clock){
 clock_=clock;
 if(pending_)stop();
 if(!active_)return;
 // The original sign-extends the global clock but zero-extends each deadline.
 auto const time{static_cast<int16_t>(clock_)};
 if(cursor_<frames.size()&&time>=frames[cursor_].deadline){
  while(cursor_<frames.size()&&time>=frames[cursor_].deadline)++cursor_;
  if(cursor_==frames.size())pending_=true;
 }
 auto const requested{frames[std::min(cursor_,frames.size()-1)].page};
 if(requested<program_pages)page_=requested;
}
}
