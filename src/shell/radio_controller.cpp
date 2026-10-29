#include "radio_controller.h"

namespace earthsiege {
radio_queue &radio_controller::queue(){return messages;}
radio_queue const &radio_controller::queue()const{return messages;}
radio_presentation_state const &radio_controller::state()const{return data;}
void radio_controller::ready(){data.ready=true;}
void radio_controller::cancel(){data.cancelling=true;}
radio_message const *radio_controller::active()const{return data.active?&messages.state().slots[*data.active]:nullptr;}
void radio_controller::advance(uint32_t const ticks,bool const text_enabled,radio_presentation_callbacks const &callbacks){
  messages.expire(ticks);
  if(!data.active && messages.state().count)data.active=messages.state().order[0];
  if(!data.active)return;
  auto &message{messages.slot(*data.active)};
  auto const remove{[&]{messages.erase(0);data.active.reset();data.cancelling=false;data.ready=false;}};
  if(data.ready || data.cancelling){
    if(data.starting && !data.cancelling){
      if(text_enabled){if(!callbacks.show(message))return;data.visible=true;}
      message.minimum+=ticks;message.maximum+=ticks;data.starting=false;return;
    }
    if(!data.cancelling && ticks<=message.maximum){
      if(ticks<=message.minimum || messages.state().count<2)return;
    }
    if(!callbacks.finish(message))return;
    data.visible=false;remove();return;
  }
  if(message.expiry<ticks){remove();data.visible=false;return;}
  if(ticks<=message.earliest || data.starting)return;
  data.starting=true;data.ready=false;callbacks.start(message);
}
} // namespace earthsiege
