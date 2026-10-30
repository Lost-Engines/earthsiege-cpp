#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>
#include "shell/mission_view.h"
#include "shell/mission_options_dialog.h"
#include "shell/mission_alert.h"
#include "resources/resource_store.h"
using namespace earthsiege;
namespace {
struct replay_sound_request {
 int32_t frame;
 uint8_t kind; // 0 start, 1 stop
 uint16_t id;
 bool operator==(replay_sound_request const &)const=default;
};
std::array<int32_t,24> camera_values(camera_state const &camera){
 return {camera.position.x,camera.position.y,camera.position.z,
                int32_t{camera.angles.x},int32_t{camera.angles.y},int32_t{camera.angles.z},
                int32_t{camera.orbit_distance},int32_t{camera.movement},
                int32_t{camera.orbit_angles.x},int32_t{camera.orbit_angles.y},int32_t{camera.orbit_angles.z},
                int32_t{camera.angular_rates.x},int32_t{camera.angular_rates.y},int32_t{camera.angular_rates.z},
                int32_t{std::to_underlying(camera.mode)},int32_t{camera.focus_offset.x},int32_t{camera.focus_offset.y},int32_t{camera.focus_offset.z},
                int32_t{camera.attachment_offset.x},int32_t{camera.attachment_offset.y},int32_t{camera.attachment_offset.z},
                int32_t{camera.look_offset.x},int32_t{camera.look_offset.y},int32_t{camera.look_offset.z}};
}
}
int main(int argc,char **argv)try{
 if(argc<3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader file{raw};
 auto magic{file.take(8)};auto version{std::string(magic.begin(),magic.end())};bool has_camera{version=="ESRPLY09"},has_radio_inputs{has_camera||version=="ESRPLY08"},visual{has_radio_inputs||version=="ESRPLY06"},terminal{visual||version=="ESRPLY05"},extended{terminal||version=="ESRPLY04"};
 if(version!="ESRPLY03"&&!extended)file.fail("format");
 auto demo{file.integer<uint32_t>()},count{file.integer<uint32_t>()};
 auto prefs{extended?file.take(17):bytes{}};
 auto expected_packets{terminal?file.integer<uint32_t>():uint32_t{0}};
 auto tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
 mission_view view{resources,tape};
 std::ofstream audio_log;int64_t audio_frame{-1};
 std::vector<replay_sound_request> audio_requests;
 std::ofstream scene_log;
 if(auto const *path{std::getenv("EARTHSIEGE_SCENE_DIAGNOSTICS")}){
  scene_log.open(path);if(!scene_log)throw std::runtime_error{"Cannot open scene diagnostics"};
  view.observe_scene([&](scene_draw_observation const &object){
   if(auto const *wanted{std::getenv("EARTHSIEGE_SCENE_FRAME")};wanted&&audio_frame!=std::stoll(wanted))return;
   scene_log<<"{\"frame\":"<<audio_frame<<",\"position\":["<<object.position.x<<','<<object.position.y<<','<<object.position.z
    <<"],\"class\":"<<object.native_class<<",\"distance\":"<<object.distance<<",\"radius\":"<<object.radius<<",\"detail\":"<<object.detail<<",\"lights\":[";
   bool first{true};
   for(auto const &value:object.lights){
    if(!first)scene_log<<',';
    first=false;
    scene_log<<'['<<std::to_underlying(value.kind)<<','<<value.intensity<<','<<value.position.x<<','<<value.position.y<<','<<value.position.z
     <<','<<value.direction.x<<','<<value.direction.y<<','<<value.direction.z<<','<<value.attenuation<<','<<value.scale<<']';
   }
   scene_log<<"]}\n";
  });
 }
 if(auto const *path{std::getenv("EARTHSIEGE_REPLAY_AUDIO")}){
  audio_log.open(path);if(!audio_log)throw std::runtime_error{"Cannot open audio diagnostics"};
 }
 view.audio_observer([&](audio_event const &e){
  if(e.kind==audio_event_kind::play||e.kind==audio_event_kind::music_play||e.kind==audio_event_kind::stop||e.kind==audio_event_kind::music_stop)
   audio_requests.push_back({static_cast<int32_t>(std::max<int64_t>(0,audio_frame)),
     static_cast<uint8_t>(e.kind==audio_event_kind::stop||e.kind==audio_event_kind::music_stop),static_cast<uint16_t>(e.id)});
  if(audio_log.is_open()){
   auto const &voices{view.audio_state().voices()};
   audio_log<<"{\"frame\":"<<audio_frame<<",\"kind\":"<<std::to_underlying(e.kind)<<",\"id\":"<<e.id<<",\"path\":"<<std::quoted(std::string(e.path))<<",\"accepted\":"<<e.accepted<<",\"voices\":"<<std::ranges::count_if(voices,[](auto const &v){return v.flags&0x8000;})<<"}\n";
  }
 });
 auto audio_clock=[&](uint32_t clock){view.audio_clock(uint64_t{clock}*2);view.take_audio();};
 preferences settings{prefs};std::array<joystick_calibration,2> calibrations;
 if(extended)view.configure_input(settings,calibrations);
 if(visual){
  auto const frames{file.integer<uint16_t>()};view.begin_startup();
  for(unsigned i{};i<frames;++i){
   auto draw_clock{file.integer<uint32_t>()},palette_clock{file.integer<uint32_t>()},end_clock{file.integer<uint32_t>()};
   if(!view.starting())file.fail("startup completed early");
   audio_clock(draw_clock);view.draw_startup(draw_clock);audio_clock(end_clock);view.finish_startup_frame(palette_clock,end_clock);
  }
  if(view.starting())file.fail("startup did not finish");
 }
 save_store replay_saves{std::filesystem::path{argv[2]}.parent_path()/"replay-settings"};
 size_t next_packet{},modal_packets{},alert_count{};bool suppressed{},exited{};
 auto alert=[&](int message){
  mission_alert dialog{resources,message};dialog.feedback([&]{view.click_sound();});
  if(next_packet>=tape.frames.size())throw std::runtime_error{"Missing alert initialization poll"};
  ++next_packet;++modal_packets;suppressed=true;
  bool left{};std::optional<int> response;
  while(!response){
   if(next_packet>=tape.frames.size())throw std::runtime_error{"Recording ended inside alert"};
   auto const &input{tape.frames[next_packet++]};++modal_packets;
   // 4cb14 dispatches queued mouse callbacks before the modal key handler.
   for(auto const &event:input.mouse){
    bool const held{bool(event.kind&1)};
    if(held&&!left)dialog.mouse_down(event.position.x,event.position.y);
    if(!held&&left)response=dialog.mouse_up(event.position.x,event.position.y);
    left=held;
   }
   if(!response)response=dialog.input(input.command,input.buttons&1,input.buttons&2,settings.current()[10]!=0).response;
  }
  ++alert_count;
  std::cout<<"Alert "<<message<<" response "<<*response<<" at packet "<<next_packet-1<<'\n';
  exited=dialog.exits_mission(*response);return exited;
 };
 auto modal=[&](uint16_t code)->mission_command_result{
  if(code==16)return alert(static_cast<int>(view.status(true)))?mission_command_result::exit_mission:mission_command_result::handled;
  if(code!=88&&code!=0x219)return mission_command_result::unhandled;
  // Original 45f00 blocks buttons and consumes one initialization poll before
  // the dialog loop. Playback skips the live debounce which would clear them.
  suppressed=true;view.begin_options();
  if(next_packet>=tape.frames.size())throw std::runtime_error{"Missing modal initialization poll"};
  ++next_packet;++modal_packets;
  mission_options_dialog options{resources,settings,replay_saves,calibrations};options.feedback([&]{view.click_sound();});
  bool left{},right{};
  while(!options.done()){
   if(next_packet>=tape.frames.size())throw std::runtime_error{"Recording ended inside preferences"};
   auto const &input{tape.frames[next_packet++]};++modal_packets;
   // These recordings exercise the original keyboard-owned preferences path.
   // Other input remains explicit rather than silently skipping a dialog.
   if(input.buttons)throw std::runtime_error{"Recorded preferences device buttons need routing"};
   options.key(input.command);
   view.step_options_recorded();view.draw();
   for(auto const &event:input.mouse){
    bool const l{bool(event.kind&1)},r{bool(event.kind&2)};
    if(l&&!left)options.mouse_down(event.position.x,event.position.y);
    if(r&&!right)options.mouse_down(event.position.x,event.position.y,true);
    if(!l&&left)options.mouse_up(event.position.x,event.position.y);
    if(!r&&right)options.mouse_up(event.position.x,event.position.y,true);
    left=l;right=r;
   }
  }
  view.apply_preferences(settings);view.end_options();return mission_command_result::handled;
 };
 std::filesystem::path diagnostic_path;
 std::ofstream diagnostics;
 if(auto const *path{std::getenv("EARTHSIEGE_REPLAY_DIAGNOSTICS")}){
  diagnostic_path=path;std::filesystem::create_directories(diagnostic_path);
  diagnostics.open(diagnostic_path/"frames.jsonl");
  if(!diagnostics)throw std::runtime_error{"Cannot open replay diagnostics"};
 }
 struct raster_frame {
  uint32_t frame;
  uint16_t x,y,width,height;
  std::vector<uint8_t> rgb;
  std::string_view kind{};
 };
 std::vector<raster_frame> raster_frames;size_t next_raster_frame{};size_t scene_frames{},instrument_frames{};
 if(has_camera)for(std::string_view kind:{"weapon","scene","instrument"}){
  auto raw_frames{read_file(std::filesystem::path{argv[2]}.parent_path()/("demo"+std::to_string(demo)+("_"+std::string{kind}+"_frames.bin")))};
  binary_reader rasters{raw_frames};auto signature{rasters.take(8)};
  if(std::string(signature.begin(),signature.end())!="ESWPF001")rasters.fail("raster frame format");
  auto const n{rasters.integer<uint32_t>()};
  for(uint32_t i{};i<n;++i){
   raster_frame value{rasters.integer<uint32_t>(),rasters.integer<uint16_t>(),rasters.integer<uint16_t>(),rasters.integer<uint16_t>(),rasters.integer<uint16_t>(),{},kind};
   if(value.x+value.width>screen_width||value.y+value.height>screen_height)rasters.fail("raster frame bounds");
   auto rgb{rasters.take(size_t{value.width}*value.height*3)};value.rgb.assign(rgb.begin(),rgb.end());
   value.kind=kind;if(kind=="scene")++scene_frames;else if(kind=="instrument")++instrument_frames;
   raster_frames.push_back(std::move(value));
  }
  rasters.require_end();
 }
 std::ranges::stable_sort(raster_frames,{},&raster_frame::frame);
 uint32_t checked{};
 for(uint32_t frame{};frame<count;++frame){
  audio_frame=int64_t{frame}-1;
  auto phase{terminal?file.integer<uint8_t>():uint8_t{1}};
  auto packet{file.integer<int32_t>()};auto clock{file.integer<uint32_t>()};auto ratio{file.integer<uint16_t>()};auto actors{file.integer<uint16_t>()};
  if(argc>3&&frame>std::stoul(argv[3]))break;
  std::vector<std::pair<radio_input_point,uint32_t>> observations;size_t observation_index{};
  if(has_radio_inputs){
   auto const n{file.integer<uint16_t>()};
   for(unsigned i{};i<n;++i){auto point{file.integer<uint8_t>()};auto ticks{file.integer<uint32_t>()};observations.emplace_back(static_cast<radio_input_point>(point),ticks);}
   view.radio_input([&](radio_input_point point,uint32_t){
    if(observation_index>=observations.size()||observations[observation_index].first!=point)
     throw std::runtime_error{"packet "+std::to_string(packet)+" radio input point "+std::to_string(std::to_underlying(point))+" at read "+std::to_string(observation_index)+" expected "+(observation_index<observations.size()?std::to_string(std::to_underlying(observations[observation_index].first)):"end")};
    return observations[observation_index++].second;
   });
  }
  std::array<int32_t,24> expected_camera{};
  if(has_camera)for(auto &value:expected_camera)value=file.integer<int32_t>();
  audio_clock(clock);
  if(packet!=-1&&packet!=-3){
   tape_frame record;
   if(packet>=0){
    if(next_packet!=size_t(packet))file.fail("modal input ownership");
    record=tape.frames.at(next_packet++);
   }
   else record.delta=ratio; // EOF clears the packet but retains the outer ratio.
   if(record.delta!=ratio)file.fail("recorded outer ratio");
   auto const complete{view.step_recorded(record,clock,suppressed,{modal,[&](mission_status status){return alert(static_cast<int>(status));}})};
   if(terminal&&((phase==2)==complete))file.fail("mission frame termination phase");
   if(complete)view.draw();
  }
  if(has_radio_inputs&&observation_index!=observations.size())throw std::runtime_error{"packet "+std::to_string(packet)+" unconsumed radio inputs: "+std::to_string(observations.size()-observation_index)};
  auto const &session{view.simulation()};
  if(phase==1)while(next_raster_frame<raster_frames.size()&&raster_frames[next_raster_frame].frame==frame-1){
   auto const &expected{raster_frames[next_raster_frame++]};size_t byte{};
   for(unsigned y{};y<expected.height;++y)for(unsigned x{};x<expected.width;++x){
    auto const index{view.image().pixels()[(expected.y+y)*screen_width+expected.x+x]};
    auto const colour{view.colours().colours[index]};
    for(auto shift:{16,8,0})if(static_cast<uint8_t>(colour>>shift)!=expected.rgb[byte++])
     throw std::runtime_error{"Original "+std::string{expected.kind}+" raster differs at frame "+std::to_string(frame-1)+
       " pixel "+std::to_string(expected.x+x)+","+std::to_string(expected.y+y)};
   }
  }
  if(diagnostics.is_open()){
   auto const &visual_state{session.interface_random().snapshot()};
   diagnostics<<"{\"row\":"<<frame<<",\"packet\":"<<packet<<",\"phase\":"<<+phase<<",\"visual\":[";
   for(size_t i{};i<visual_state.words.size();++i){if(i)diagnostics<<',';diagnostics<<visual_state.words[i];}
   diagnostics<<','<<visual_state.first<<','<<visual_state.second<<"],\"camera\":[";
    auto const &camera{view.view_camera()};
    auto const values{camera_values(camera)};
    for(size_t i{};i<values.size();++i){if(i)diagnostics<<',';diagnostics<<values[i];}
    diagnostics<<"],\"clouds\":[";
    auto const &clouds{view.cloud_state()};auto const &anchor{clouds.anchor()};auto const &wind{clouds.wind()};
    for(auto v:{anchor.x,anchor.y,anchor.z,wind.x,wind.y,wind.z})diagnostics<<v<<',';
    for(auto const &p:clouds.points())diagnostics<<p.x<<','<<p.y<<','<<p.z<<',';
    diagnostics<<"0],\"weapons\":{\"selection\":"<<session.player_weapon_controls().selection()<<",\"rows\":[";
    bool first_row{true};
    for(auto const &row:session.player_weapon_controls().displays()){
     if(!first_row)diagnostics<<',';
     first_row=false;
     auto const &state{row.state};
     diagnostics<<'['<<row.bank_slot<<','<<row.control_number<<','<<+std::to_underlying(row.kind)<<','
       <<state.value<<','<<state.requested_charge<<','<<state.disabled<<','<<state.selected<<','<<state.included<<','<<state.ready<<']';
    }
    diagnostics<<"]},\"lights\":[";
    bool first_light{true};
    for(auto const *value:view.active_lights()){
     if(!first_light)diagnostics<<',';
     first_light=false;
     diagnostics<<'['<<std::to_underlying(value->kind)<<','<<value->intensity<<','
      <<value->position.x<<','<<value->position.y<<','<<value->position.z<<','
      <<value->direction.x<<','<<value->direction.y<<','<<value->direction.z<<','
      <<value->attenuation<<','<<value->scale<<']';
    }
    diagnostics<<"]}\n";
   if(phase==1&&(frame-1)%60==0){
    std::ostringstream name;name<<"frame-"<<std::setw(5)<<std::setfill('0')<<frame-1<<".ppm";
    auto path{diagnostic_path/name.str()};view.image().save(path,view.colours());
    auto const pixels{view.image().pixels()};
    std::ofstream raster{path.replace_extension(".bin"),std::ios::binary};
    raster.write(reinterpret_cast<char const *>(pixels.data()),static_cast<std::streamsize>(pixels.size()));
    std::ofstream pal{path.replace_extension(".pal"),std::ios::binary};
    for(auto const &c:view.colours().dac_colours)pal.write(reinterpret_cast<char const *>(c.data()),3);
   }
  }
  if(has_camera){
   auto const actual{camera_values(view.view_camera())};
   for(size_t i{};i<actual.size();++i)if(actual[i]!=expected_camera[i])
    throw std::runtime_error{"frame "+std::to_string(frame)+" camera component "+std::to_string(i)+": native "+std::to_string(actual[i])+", original "+std::to_string(expected_camera[i])};
  }
  auto const &rng{session.random_snapshot()};
  random_state::state expected;for(auto &v:expected.words)v=file.integer<uint16_t>();expected.first=file.integer<uint8_t>();expected.second=file.integer<uint8_t>();
  size_t errors{};auto check=[&](std::string const &label,auto actual,auto want){
   if(actual!=want){if(errors<25)std::cerr<<"packet "<<packet<<' '<<label<<": native "<<+actual<<", original "<<+want<<'\n';++errors;}
  };
  check("random first",rng.first,expected.first);check("random second",rng.second,expected.second);
  for(size_t i{};i<rng.words.size();++i)check("random word "+std::to_string(i),rng.words[i],expected.words[i]);
  if(visual){
   auto const &actual{session.interface_random().snapshot()};
   for(size_t i{};i<actual.words.size();++i)check("visual word "+std::to_string(i),actual.words[i],file.integer<uint16_t>());
   check("visual first",actual.first,file.integer<uint8_t>());check("visual second",actual.second,file.integer<uint8_t>());
  }
  auto hercs{session.hercs()};auto flyers{session.flying_actors()};auto bases{session.ground_actors()};size_t h{},f{},b{};
  check("actor count",hercs.size()+flyers.size()+bases.size(),size_t(actors));
  for(unsigned a{};a<actors;++a){
   std::array<uint8_t,0x250> storage{};
   for(auto const [at,n]:std::array<std::pair<size_t,size_t>,10>{{{0,2},{6,6},{32,12},{0x4c,4},{0x90,1},{0x93,1},{0x9e,1},{0x1b8,4},{0x222,8},{0x22e,4}}}){
    auto part{file.take(n)};std::copy(part.begin(),part.end(),storage.begin()+at);
   }
   if(extended)for(auto const [at,n]:std::array<std::pair<size_t,size_t>,14>{{{0x50,4},{0x7c,2},{0x8f,1},{0x97,2},{0x9c,1},{0xa6,1},{0x15c,2},{0x179,2},{0x17c,2},{0x1d2,2},{0x238,2},{0x22a,4},{0xbc,actors},{0x10c,actors}}}){
    auto part{file.take(n)};std::copy(part.begin(),part.end(),storage.begin()+at);
   }
   bytes data{storage};auto get=[&]<class T>(size_t at){binary_reader r{data.subspan(at)};return r.integer<T>();};
   auto kind{get.template operator()<uint16_t>(0)};
   auto expected_target{file.integer<int32_t>()};auto expected_controller{file.integer<int16_t>()};
   auto const &relationship{session.activity_state().targets()[a]};
   auto const &controller{session.activity_state().decisions()[a].controller};
   check("actor "+std::to_string(a)+" target",relationship.selected?int32_t(*relationship.selected):-1,expected_target);
   if(extended){
    auto const &awareness{session.activity_state().actor(a).awareness};
    std::string label{"actor "+std::to_string(a)+" "};
    auto byte=[&](size_t at){return get.template operator()<uint8_t>(at)!=0;};
    check(label+"known size",awareness.known.size(),size_t(actors));
    for(size_t i{};i<actors;++i){
     check(label+"known "+std::to_string(i),awareness.known[i],get.template operator()<uint8_t>(0xbc+i));
     check(label+"visible "+std::to_string(i),awareness.visible[i],get.template operator()<uint8_t>(0x10c+i));
    }
    check(label+"seen",awareness.seen,byte(0x8f));
    check(label+"alerted",awareness.alerted,byte(0x98));
    check(label+"event busy",awareness.event_busy,byte(0x9c));
    check(label+"visibility clock",awareness.visibility_remaining,get.template operator()<int16_t>(0x179));
    check(label+"refresh clock",awareness.refresh_remaining,get.template operator()<int16_t>(0x17c));
    check(label+"attackers",relationship.attackers,get.template operator()<int16_t>(0x15c));
    check(label+"target changed",relationship.changed,byte(0x97));
    if(kind!=5){
     check(label+"controller updates",controller.ticks,get.template operator()<uint32_t>(0x50));
     check(label+"controller updated",controller.updated,byte(0x7c));
     check(label+"controller first",controller.first_entry,byte(0x7d));
    }
    if(kind==7){
     auto const &decision{session.activity_state().decisions()[a]};
     check(label+"command",decision.command,get.template operator()<int16_t>(0x1d2));
     // Original +0x238 is untouched by construction. Compare only when an
     // attacking AI consumer has chosen a component, not allocator residue.
     if(!session.activity_state().actor(a).player&&controller.type&&describe(*controller.type).attacking()&&relationship.selected&&session.activity_state().actor(*relationship.selected).kind==actor_kind::herc)
      check(label+"aimed component",decision.aimed_component,get.template operator()<int16_t>(0x238));
     check(label+"attack request",decision.attack_requested,byte(0xa6));
    }
   }
   if(kind!=5){
    check("actor "+std::to_string(a)+" controller",controller.type?int(std::to_underlying(*controller.type)):-1,int(expected_controller));
    check("actor "+std::to_string(a)+" controller clock",controller.reconsider_remaining,get.template operator()<int32_t>(0x4c));
   }vec3<int32_t> position;vec3<uint16_t> angles;
   std::string label{"actor "+std::to_string(a)+" "};
   if(kind==7){
    auto const &actor{hercs.at(h++).actor};auto const &body{actor.body()};auto const &motion{body.movement()};
    position=motion.position;angles=motion.angles;
    auto const &damage{actor.damage()};
    check(label+"dead",damage.consequences.dead,get.template operator()<uint8_t>(0x93)!=0);
    check(label+"disabled",damage.consequences.disabled,get.template operator()<uint8_t>(0x9e)!=0);
    check(label+"detectable",actor.impacts().detectable,get.template operator()<uint8_t>(0x90)!=0);
    for(auto const &values:{damage.internal,damage.armour,damage.present}){
     auto n{file.integer<uint16_t>()};check(label+"damage size",values.size(),size_t(n));
     for(size_t j{};j<n;++j)check(label+"damage "+std::to_string(j),values.at(j),file.integer<int16_t>());
    }
    if(extended){
     check(label+"torso rate",body.torso().rate,get.template operator()<int16_t>(0x22a));
     check(label+"elevation rate",body.elevation().rate,get.template operator()<int16_t>(0x22c));
    }
    check(label+"steering",motion.steering_rate,get.template operator()<int16_t>(0x222));
    check(label+"gait",motion.gait_rate,get.template operator()<int16_t>(0x224));
    check(label+"throttle",motion.throttle,get.template operator()<int16_t>(0x226));
    check(label+"torso",body.torso().angle,get.template operator()<int16_t>(0x22e));
    check(label+"elevation",body.elevation().angle,get.template operator()<int16_t>(0x230));
    check(label+"power",actor.power().reserve,get.template operator()<int16_t>(0x228));
    check(label+"front shield",actor.power().shields.front,get.template operator()<int16_t>(0x1b8));
    check(label+"rear shield",actor.power().shields.rear,get.template operator()<int16_t>(0x1ba));
   }else if(kind==6){auto const &actor{flyers.at(f++).actor};position=actor.motion().position;angles=actor.angles();check(label+"flyer dead",actor.dead(),get.template operator()<uint8_t>(0x93)!=0);}
   else if(kind==5){auto const &actor{bases.at(b++).actor};position=actor.placement().translation;angles={get.template operator()<uint16_t>(6),get.template operator()<uint16_t>(8),get.template operator()<uint16_t>(10)};}
   else file.fail("actor kind");
   for(unsigned axis{};axis<3;++axis){
    check(label+"position "+std::to_string(axis),position[axis],get.template operator()<int32_t>(32+axis*4));
    if(kind!=5)check(label+"angle "+std::to_string(axis),angles[axis],get.template operator()<uint16_t>(6+axis*2));
   }
  }
  auto ng{file.integer<uint16_t>()};auto const &activity{session.activity_state()};
  check("group count",activity.all_groups().size(),size_t(ng));
  for(size_t g{};g<ng;++g){
   auto data{file.take(40)};auto const &group{activity.group(g)};
   binary_reader r{data.subspan(4)};check("group order",group.order,r.integer<int16_t>());check("group route",group.route_cursor,r.integer<int16_t>());
  }
  if(errors){std::cerr<<"Native HUD target "<<(session.target_selection()?int(*session.target_selection()):-1)<<'\n';std::cerr<<errors<<" differing fields\n";return 1;}
  ++checked;
  if(frame%100==0)std::cout<<frame<<" original states agree\n";
 }
 if(argc==3){
  if(next_raster_frame!=raster_frames.size())throw std::runtime_error{"Unconsumed raster observations"};
  if(!raster_frames.empty())std::cout<<raster_frames.size()-scene_frames-instrument_frames<<" original weapon-name rectangles, "<<scene_frames<<" scene rasters and "<<instrument_frames<<" gauge samples agree\n";
  if(terminal){
   auto compare_file=[&](char const *name,std::vector<uint8_t> const &actual){
    auto expected{file.take(file.integer<uint32_t>())};
    if(actual.size()!=expected.size())throw std::runtime_error{std::string(name)+" size: native "+std::to_string(actual.size())+", original "+std::to_string(expected.size())};
    for(size_t i{};i<actual.size();++i)if(actual[i]!=expected[i])
     throw std::runtime_error{std::string(name)+" byte "+std::to_string(i)+": native "+std::to_string(actual[i])+", original "+std::to_string(expected[i])};
   };
   compare_file("RESULTS.DAT",write_mission_results(view.results()));
   compare_file("MISSION.VAR",write_mission_counters(view.mission_variables()));
  }
  file.require_end();
  if(has_radio_inputs){
   auto audio_raw{read_file(std::filesystem::path{argv[2]}.parent_path()/("demo"+std::to_string(demo)+"_audio.bin"))};
   binary_reader sounds{audio_raw};auto signature{sounds.take(8)};
   if(std::string(signature.begin(),signature.end())!="ESREQ002")sounds.fail("audio format");
   std::vector<replay_sound_request> expected;
   auto const requests{sounds.integer<uint32_t>()};
   for(uint32_t i{};i<requests;++i){
    auto const frame{sounds.integer<int32_t>()};auto const kind{sounds.integer<uint8_t>()};auto const id{sounds.integer<uint16_t>()};
    expected.push_back({frame,kind,id});
   }
   sounds.require_end();
   // Engine start retries depend on the separately clocked PCM completion and
   // voice allocator. Retain them in the fixture, but compare other requests
   // exactly in frame/call order without injecting any expected request.
   auto engine=[](auto const &request){return request.kind==0&&(request.id==16||request.id==45);};
   std::erase_if(expected,engine);std::erase_if(audio_requests,engine);
   if(expected!=audio_requests){
    size_t i{};while(i<expected.size()&&i<audio_requests.size()&&expected[i]==audio_requests[i])++i;
    throw std::runtime_error{"Ordered sound start/stop request differs at "+std::to_string(i)};
   }
   std::cout<<expected.size()<<" ordered non-engine starts and all stops agree\n";
  }
  if(terminal&&next_packet!=expected_packets)file.fail("recorded input consumption");
  if(terminal&&expected_packets!=tape.frames.size())file.fail("source TAP packet count");
  if(terminal&&alert_count&&!exited)file.fail("terminal dialog did not leave mission");
 }
 std::cout<<modal_packets<<" packets consumed by native modal controls\n";
 std::cout<<"DEMO"<<demo<<": "<<checked<<" original states agree (including initial/EOF/exit when present)\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
