#include "shell/game_shell.h"
#include "shell/mission_view.h"
#include <chrono>
#include <iostream>
using namespace earthsiege;
void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
struct cleanup{std::filesystem::path path;~cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}};
int main(int argc,char **argv)try{
 if(argc!=4)return 1;
 resource_store resources{argv[1]};
 cleanup temp{std::filesystem::path{argv[2]}/("flight-options-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
 save_store files{temp.path};files.save_preferences(files.load_preferences());game_shell shell{resources,temp.path,17};
 mission_view scene{resources,1};scene.configure_input(shell.settings(),shell.device_calibrations());
 scene.advance_time(25);require(scene.step(false,false),"live mission started");scene.draw();
 scene.begin_options();
 auto options{shell.flight_options()};mission_preferences_artwork artwork{resources};
 control_settings_artwork controls{resources,true};calibration_artwork calibration{resources,true};
 auto click=[&](rectangle b,bool right=false){int x{(b.left+b.right)/2},y{(b.top+b.bottom)/2};options->mouse_down(x,y,right);options->mouse_up(x,y,right);};
 auto before{shell.settings().current()};
 click(artwork.button(0));require(shell.settings().current()[0]==0,"music changed live");
 click(artwork.button(4),true);require(shell.settings().current()[4]==223,"right click decrements sky detail");
 options->key(13);require(shell.settings().current()[4]==255,"plus raises focused detail");
 click(artwork.button(6));require(shell.settings().current()[6]==0,"detail wraps at maximum");
 options->key(12);require(shell.settings().current()[6]==255,"minus wraps at zero");
 click(artwork.button(7));require(options->child_active(),"controls child opened");
 click(controls.button(0));require(shell.settings().current()[7]==1,"device changes through in-flight controls");
 std::array<joystick_hardware,2> hardware;hardware[0].present=true;options->devices(hardware);
 click(controls.button(5));click(calibration.button(1));
 hardware[0].axes={1000,2000,32768,32768};options->devices(hardware);options->key(28);
 hardware[0].axes={60000,61000,32768,32768};options->devices(hardware);options->key(28);
 hardware[0].axes={30000,31000,32768,32768};options->devices(hardware);options->key(28);
 auto const calibrated{shell.device_calibrations()[0]};
 require(calibrated.minimum[0]==1000&&calibrated.maximum[1]==61000,"in-flight calibration uses device samples");
 click(calibration.button(2));options->key(1);
 require(!options->child_active()&&!options->done()&&options->focused()==7,"Escape from controls returns to flight options");
 auto saved{files.load_preferences().current()};
 require(saved[0]==before[0]&&saved[7]==1,"controls saves only device preferences");
 scene.apply_preferences(shell.settings());scene.draw();
 framebuffer frame{scene.image()};options->draw(frame);
 frame.save(std::filesystem::path{argv[2]}/"flight-options-live.ppm",scene.colours());
 options->key(1);require(options->done(),"Escape accepts flight options");
 saved=files.load_preferences().current();
 require(saved[0]==before[0]&&shell.settings().current()[0]==0,"original Controls commit can suppress earlier flight-option writes");
 options=shell.flight_options();click(artwork.button(1));options->key(1);
 saved=files.load_preferences().current();require(saved==shell.settings().current(),"a further flight edit saves all seven flight slots together");
 game_shell reopened{resources,temp.path,19};
 require(reopened.settings().current()==saved&&reopened.device_calibrations()[0]==calibrated,"settings and calibration survive restart");
 scene.end_options();scene.configure_input(shell.settings(),shell.device_calibrations());
 scene.advance_time(25);require(scene.step(false,false),"mission resumes after options");
 mission_audio audio{resources};random_state rng;music_contacts contacts;
 require(audio.play(45),"engine started");
 audio.preferences(false,false,false);
 require(!audio.playing(45)&&!audio.play(45),"effects switch stops active effects and gates future starts");
 auto initial_rng{rng.snapshot()};audio.update_music(contacts,rng);
 require(audio.music_status().track==-1&&rng.snapshot().words==initial_rng.words&&rng.snapshot().first==initial_rng.first,"disabled music consumes no selection RNG");
 auto const &speech_volume{resources.archives().at("simvoice")};auto speech{speech_volume.entries().begin()->first};
 require(!audio.speech(speech),"text-only radio blocks new speech");
 audio.preferences(true,false,true);audio.update_music(contacts,rng);
 require(audio.music_status().track>=0,"music can start after enabling");
 require(audio.speech(speech),"radio speech remains independent of effects gate");
 audio.preferences(true,false,false);require(audio.playing(47),"radio switch does not interrupt an existing phrase");
 audio.preferences(true,true,true);audio.preferences(true,false,true);
 require(!audio.playing(47),"falling effects switch stops already registered PCM speech");
 auto fixture{read_file(argv[3])};binary_reader f{fixture};auto magic{f.take(8)};
 require(std::string(magic.begin(),magic.end())=="ESPRFN01","nested fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  preferences p{f.take(17)};auto actions{f.integer<uint16_t>()};
  save_store saved_case{temp.path/("nested-"+std::to_string(i))};saved_case.save_preferences(p);
  auto cal{load_host_calibrations(saved_case)};mission_options_dialog dialog{resources,p,saved_case,cal};
  auto check=[&]{
   auto disk{saved_case.load_preferences()};
   for(auto values:{p.current(),p.saved(),p.previous(),disk.current()})
    for(auto v:values)if(f.integer<uint8_t>()!=v)f.fail("original nested preference transaction");
  };
  check();
  for(unsigned j{};j<actions;++j){
   auto index{f.integer<uint8_t>()};bool reverse{f.integer<uint8_t>()!=0};auto b{artwork.button(index)};
   int x{(b.left+b.right)/2},y{(b.top+b.bottom)/2};dialog.mouse_down(x,y,reverse);dialog.mouse_up(x,y,reverse);
   if(index==7)dialog.key(1);
   check();
  }
 }
 f.require_end();
 std::cout<<"Flight options, nested device calibration, persistence, resume and independent audio switches pass\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
