#include "shell_door.h"
namespace earthsiege {
shell_door::shell_door(bytes data,framebuffer const &base,palette const &colours)
 :animation{data},frame{animation.width(),animation.height(),0,{}},saved{base.pixels().begin(),base.pixels().end()},palette_{colours}{
 if(frame.width!=320||frame.height!=200)throw std::runtime_error{"menu transition dimensions"};
 frame.pixels=saved;advance();
}
void shell_door::advance(){
 if(next==animation.frames()){done=true;return;}
 animation.decode(next++,frame.pixels,saved);
}
}
