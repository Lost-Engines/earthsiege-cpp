#include "shell/mission_view.h"
#include "shell/mission_alert.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc<3)return 1;resource_store resources{argv[1]};auto demo{std::stoi(argv[2])};
 auto tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")),resources.edition())};mission_view view{resources,tape};
 preferences::values defaults{1,1,2,1,255,255,255,0,0,0,0,1,0,0,2,0,0,1};auto settings{preferences{defaults,true,resources.edition()}};view.configure_input(settings,{});
 size_t next{},frames{},modals{},queued{};uint64_t clock{};bool exited{};
 auto alert=[&](int message){
  mission_alert dialog{resources,message};if(next<tape.frames.size()){++next;++modals;}
  bool left{};std::optional<int> answer;
  while(!answer&&next<tape.frames.size()){
   auto const &input{tape.frames[next++]};++modals;
   for(auto const &e:input.mouse){bool held{bool(e.kind&1)};if(held&&!left)dialog.mouse_down(e.position.x,e.position.y);if(!held&&left)answer=dialog.mouse_up(e.position.x,e.position.y);left=held;}
   if(!answer)answer=dialog.input(input.command,input.buttons&1,input.buttons&2,settings.current()[10]!=0).response;
  }
  exited=answer&&dialog.exits_mission(*answer);return exited;
 };
 mission_dialog_handlers dialogs{.command{[&](uint16_t code){if(code==16)return alert(static_cast<int>(view.status(true)))?mission_command_result::exit_mission:mission_command_result::handled;return mission_command_result::unhandled;}},.outcome{[&](mission_status status){return alert(static_cast<int>(status));}}};
 while(next<tape.frames.size()&&!exited){
  auto const &record{tape.frames[next++]};queued+=record.commands.size();clock+=static_cast<uint64_t>(record.delta)*100/256;
  if(!view.step_recorded(record,static_cast<uint32_t>(clock*60/1000),false,dialogs))exited=true;
  view.audio_clock(clock*120/1000);view.take_audio();
  if(frames%64==0||exited){view.draw();if(argc>3){std::filesystem::path path{argv[3]};std::filesystem::create_directories(path);view.image().save(path/("demo"+std::to_string(demo)+"-"+std::to_string(frames)+".ppm"),view.colours());}}
  ++frames;
 }
 if(next!=tape.frames.size())throw std::runtime_error{"Replay exited before consuming all recorded input"};
 if(frames<1000)throw std::runtime_error{"Replay ended before sustained simulation"};
 std::cout<<"Expansion demo "<<demo<<": "<<frames<<" simulation frames, "<<modals<<" modal packets, "<<queued<<" queued commands; consumed "<<next<<"/"<<tape.frames.size()<<" packets\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
