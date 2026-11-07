#include "demo_playback.h"
#include <stdexcept>

namespace earthsiege {
namespace {
input_tape load_demo(resource_store const &resources,unsigned demo){
  if(demo<1||demo>4)throw std::runtime_error{"demo must be between 1 and 4"};
  auto const name{"TAPES/DEMO"+std::to_string(demo)+".TAP"};
  auto const path{resources.external(name)};
  if(!path)throw std::runtime_error{"Missing demo recording: "+name};
  return read_input_tape(read_file(*path),resources.edition());
}
}
demo_playback::demo_playback(resource_store const &r,unsigned demo,preferences const &prefs,
                             std::filesystem::path save_directory,music_settings const &music)
 :resources{r},tape{load_demo(r,demo)},view_{r,tape,music},settings{prefs},saves{std::move(save_directory),r.edition()}{
  calibrations=load_host_calibrations(saves);
  view_.configure_input(settings,calibrations);view_.begin_startup();
}
unsigned demo_playback::advance(uint16_t delta){
  auto const before{elapsed_units/256};elapsed_units+=uint64_t{delta}*100;
  auto const milliseconds{static_cast<unsigned>(elapsed_units/256-before)};
  view_.advance_time(milliseconds);return milliseconds;
}
bool demo_playback::present(presenter const &show,unsigned milliseconds,framebuffer const &frame){
  if(!show(milliseconds,frame))finished=true;
  return !finished;
}
bool demo_playback::alert(int message,presenter const &show){
  mission_alert dialog{message==-1?mission_alert{resources,view_.simulation().objective_text}:mission_alert{resources,message}};dialog.feedback([&]{view_.click_sound();});
  // 45f00 consumes an initialization poll before dispatching modal input.
  if(next==tape.frames.size()){finished=true;return true;}
  ++next;++modal_packets;suppressed=true;
  bool left{};std::optional<int> response;
  while(!response&&!finished){
    if(next==tape.frames.size()){finished=true;break;}
    auto const &input{tape.frames[next++]};++modal_packets;
    auto milliseconds{advance(input.delta)};
    for(auto const &event:input.mouse){
      bool const held{bool(event.kind&1)};
      if(held&&!left)dialog.mouse_down(event.position.x,event.position.y);
      if(!held&&left)response=dialog.mouse_up(event.position.x,event.position.y);
      left=held;
    }
    if(!response)response=dialog.input(input.command,input.buttons&1,input.buttons&2,settings.current()[10]!=0).response;
    auto frame{view_.image()};dialog.draw(frame);present(show,milliseconds,frame);
  }
  if(response&&dialog.exits_mission(*response))finished=true;
  return finished;
}
mission_command_result demo_playback::command(uint16_t code,presenter const &show){
  using enum mission_command_result;
  if(code==87&&view_.view_camera().mode==camera_mode::attached)return alert(-1,show)?exit_mission:handled;
  if(code==16)return alert(static_cast<int>(view_.status(true)),show)?exit_mission:handled;
  if(code==25||code==0x401||code==0x410)return alert(code==25?0:1,show)?exit_mission:handled;
  if(code!=88&&code!=0x219)return unhandled;
  suppressed=true;view_.begin_options();
  if(next==tape.frames.size()){finished=true;return exit_mission;}
  ++next;++modal_packets;
  mission_options_dialog options{resources,settings,saves,calibrations};options.feedback([&]{view_.click_sound();});
  bool left{},right{};
  while(!options.done()&&!finished){
    if(next==tape.frames.size()){finished=true;break;}
    auto const &input{tape.frames[next++]};++modal_packets;
    auto milliseconds{advance(input.delta)};
    options.key(input.command);
    view_.step_options_recorded();view_.draw();
    for(auto const &event:input.mouse){
      bool const l{bool(event.kind&1)},r{bool(event.kind&2)};
      if(l&&!left)options.mouse_down(event.position.x,event.position.y);
      if(r&&!right)options.mouse_down(event.position.x,event.position.y,true);
      if(!l&&left)options.mouse_up(event.position.x,event.position.y);
      if(!r&&right)options.mouse_up(event.position.x,event.position.y,true);
      left=l;right=r;
    }
    auto frame{view_.image()};options.draw(frame);present(show,milliseconds,frame);
  }
  view_.apply_preferences(settings);view_.end_options();
  return finished?exit_mission:handled;
}
void demo_playback::step(presenter const &show){
  if(finished)return;
  if(view_.starting()){
    elapsed_units+=17*256;view_.advance_time(17);view_.step_startup();present(show,17,view_.image());return;
  }
  bool const eof{next==tape.frames.size()};
  tape_frame empty;empty.delta=last_delta;
  auto const &record{eof?empty:tape.frames[next++]};last_delta=record.delta;
  auto const milliseconds{advance(record.delta)};
  bool const complete{view_.step_recorded(record,static_cast<uint32_t>(elapsed_units*60/256000),suppressed,
    {.command{[&](uint16_t code){return command(code,show);}},
     .outcome{[&](mission_status status){return alert(static_cast<int>(status),show);}}})};
  ++simulation_frames;
  if(complete&&!finished){view_.draw();present(show,milliseconds,view_.image());}
  // Original playback performs one zero-input update at EOF, retaining ratio.
  if(eof||!complete)finished=true;
}
}
