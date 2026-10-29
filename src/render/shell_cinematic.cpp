#include "shell_cinematic.h"
namespace earthsiege {
shell_cinematic::shell_cinematic(resource_store const &r,std::string const &path,random_state &random)
 :player{read_presentation(r.read("shell0",path))},art{r,path.substr(0,path.find_last_of('/')+1)}{execute(0,random);present();}
void shell_cinematic::execute(int16_t elapsed,random_state &random){
 visible=background;player.advance(elapsed,random);art.apply(player.commands(),visible,background);
 for(auto const &frame:art.palette_frames())pending.push_back(frame);
 pending.push_back({visible,art.colours()});
}
void shell_cinematic::present(){
 if(pending.empty())return;
 shown=std::move(pending.front().image);colours_=std::move(pending.front().colours);pending.pop_front();
}
void shell_cinematic::advance(random_state &random){
 if(pending.empty())execute(1,random);
 present();
}
}
