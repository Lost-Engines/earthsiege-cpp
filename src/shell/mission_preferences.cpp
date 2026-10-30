#include "mission_preferences.h"
#include <stdexcept>
namespace earthsiege {
mission_preference_action mission_preferences::activate(unsigned i,bool reverse){
 switch(i){
 case 0:case 1:case 3:prefs.toggle(i);break;
 case 2:if(radio_available)prefs.set(2,static_cast<uint8_t>((prefs.current()[2]+1)%3));break;
 case 4:case 5:case 6:if(reverse)prefs.decrease(i,32);else prefs.increase(i,32);break;
 case 7:return mission_preference_action::controls;
 case 8:for(size_t j{};j<7;++j)persisted_[j]=prefs.current()[j];return mission_preference_action::done;
 default:throw std::out_of_range{"mission preference"};
 }
 return mission_preference_action::none;
}
}
