#include "frame_clock.h"
#include "fixed_math.h"
namespace earthsiege {
void frame_clock::advance(unsigned ticks){
 for(unsigned i{};i<ticks;++i){
  if(state_.countdown)state_.countdown=wrap_word(int(state_.countdown)-1);
  if(state_.fast++&1)++state_.slow;
 }
}
bool frame_clock::ready()const{return state_.countdown<=wrap_word(int(state_.maximum)-2);}
std::optional<frame_ratio> frame_clock::take(){
 if(!ready())return {};
 state_.elapsed=wrap_word(int(state_.maximum)-state_.countdown);
 state_.ratio=ratio_from_period(state_.elapsed,state_.nominal);
 state_.alternate_ratio=ratio_from_period(state_.elapsed,state_.alternate_nominal);
 state_.countdown=state_.maximum;return state_.ratio;
}
}
