#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"
namespace {
earthsiege::random_state::state random_input(earthsiege::binary_reader &file) {
  earthsiege::random_state::state r;for(auto &x:r.words)x=file.integer<uint16_t>();r.first=file.integer<uint8_t>();r.second=file.integer<uint8_t>();return r;
}
std::vector<int16_t> words(earthsiege::binary_reader &file) {
  std::vector<int16_t> r(file.integer<uint16_t>());for(auto &v:r)v=file.integer<int16_t>();return r;
}
}
int main(int argc,char **argv) try {
  using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  auto const version{std::string(magic.begin(),magic.end())};
  bool const authored{version=="ESWFRM06" || version=="ESWFRM07"};
  bool const interactive{version=="ESWFRM05" || version=="ESWFRM07"};
  bool const player_input_frames{version=="ESWFRM04" || interactive};
  bool const extended{version=="ESWFRM03" || player_input_frames || authored};
  if(!extended && version!="ESWFRM02")file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};auto const expected_frames{file.integer<uint32_t>()};size_t frames{0},states{0};
  for(size_t trial{0};trial!=count;++trial) {
    auto const demo{file.integer<uint16_t>()};bool const friendly{file.integer<uint8_t>()!=0};auto const ratio{file.integer<int16_t>()};auto const initial{random_input(file)};
    input_tape tape;std::string scenario{std::to_string(demo)};
    if(authored){
      scenario=file.cstring();
      for(size_t i{};i!=3;++i){auto data=file.take(file.integer<uint32_t>());tape.starting_files[i].assign(data.begin(),data.end());}
    }else tape=read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")));
    auto script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    if(!authored)for(auto &g:script.groups) {if(friendly)g.mode=0;g.trigger=-1;}
    auto roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    if(interactive) {
      auto const subtype{file.integer<int16_t>()};
      if(subtype>=0) { auto &player{roster.hercs.at(static_cast<size_t>(roster.selected))};
        for(size_t slot{0};slot!=player.weapons.size();++slot)if(player.weapons[slot]>=9 && player.weapons[slot]<=11)player.weapon_values[slot]=subtype;
      }
    }
    mission_session session{resources,script,roster,random_state{initial},counters};
    auto const steps{file.integer<uint16_t>()},nh{file.integer<uint16_t>()},nf{file.integer<uint16_t>()};
    for(size_t step{0};step!=steps;++step) {
      if(player_input_frames) {
        player_input input;
        input.movement={file.integer<int16_t>(),file.integer<int16_t>()};
        input.aiming={file.integer<int16_t>(),file.integer<int16_t>()};
        input.camera={file.integer<int16_t>(),file.integer<int16_t>()};
        input.mouse={file.integer<int32_t>(),file.integer<int32_t>()};
        input.left=file.integer<uint8_t>()!=0;input.right=file.integer<uint8_t>()!=0;
        input.fire=file.integer<uint8_t>()!=0;input.enter=file.integer<uint8_t>()!=0;
        input.command=file.integer<uint16_t>();
        if(interactive) {
          auto const target{file.integer<int32_t>()};if(target!=-2)session.set_displayed_target(target<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(target)});
          auto const slot{file.integer<int16_t>()};if(slot>=0)session.select_weapon(static_cast<size_t>(slot));
          auto const tracking{file.integer<int8_t>()};if(tracking>=0)session.set_auto_tracking(tracking!=0);
          if(file.integer<uint8_t>())session.toggle_weapon_pair();
          input.camera_source=static_cast<player_axis_pair>(file.integer<uint8_t>());
        }
        session.step_player(input,ratio);session.update_player_interface();
      } else session.step({},ratio);
      auto const hercs{session.hercs()};auto const flyers{session.flying_actors()};
      if(hercs.size()!=nh || flyers.size()!=nf)file.fail("actor count");
      bool matches{true};
      auto const prefix{scenario+"/"+std::to_string(ratio)+" frame "+std::to_string(step)};
      for(size_t i{0};i!=nh;++i) {
        auto const label{prefix+" HERC "+std::to_string(i)};
        auto const &actor{hercs[i].actor};auto const &body{actor.body()};auto const &motion{body.movement()};
        auto const &decision{session.activity_state().decisions()[i]};auto const &controller{decision.controller};
        auto const check{[&](std::string const &field,auto expected,auto actual){
          if(expected!=actual) {std::cerr<<label<<' '<<field<<": expected "<<expected<<", actual "<<actual<<'\n';matches=false;}
        }};
        for(unsigned axis{0};axis!=3;++axis)check("position["+std::to_string(axis)+"]",file.integer<int32_t>(),motion.position[axis]);
        for(unsigned axis{0};axis!=3;++axis)check("angles["+std::to_string(axis)+"]",file.integer<uint16_t>(),motion.angles[axis]);
        check("steering",file.integer<int16_t>(),motion.steering_rate);check("gait",file.integer<int16_t>(),motion.gait_rate);
        check("throttle",file.integer<int16_t>(),motion.throttle);check("playback",file.integer<int16_t>(),motion.playback_rate);
        check("torso rate",file.integer<int16_t>(),body.torso().rate);check("elevation rate",file.integer<int16_t>(),body.elevation().rate);
        check("torso",file.integer<int16_t>(),body.torso().angle);check("elevation",file.integer<int16_t>(),body.elevation().angle);
        check("controller",file.integer<uint16_t>(),controller.type?std::to_underlying(*controller.type):-1);
        check("clock",file.integer<int32_t>(),controller.reconsider_remaining);check("ticks",file.integer<uint32_t>(),controller.ticks);
        auto const timer{file.integer<int32_t>()};
        if(controller.type==behavior::patrolling || controller.type==behavior::search_destroy || controller.type==behavior::guarding)
          check("search timer",timer,controller.target_search_remaining);
        check("route",file.integer<int16_t>(),session.activity_state().group(hercs[i].group).route_cursor);
        check("recovery",file.integer<int32_t>(),motion.collision_recovery);file.integer<int16_t>();
        check("reverse",file.integer<uint8_t>()!=0,motion.reverse_recovery);
        auto const &damage{actor.damage()};auto const &c{damage.consequences};
        bool const flags[]{actor.impacts().detectable,c.dead,c.disabled,decision.unavailable,c.movement_light,c.movement_heavy,c.reactor_light,c.reactor_heavy,motion.collapsed};
        for(size_t j{0};j!=9;++j)check("flag "+std::to_string(j),file.integer<uint8_t>()!=0,flags[j]);
        check("power",file.integer<int16_t>(),actor.power().reserve);auto const &shields{actor.power().shields};
        check("front shield",file.integer<int16_t>(),shields.front);check("rear shield",file.integer<int16_t>(),shields.rear);
        check("shield balance",file.integer<int16_t>(),shields.balance);check("shield capacity",file.integer<int16_t>(),shields.capacity);
        check("strength",file.integer<int16_t>(),actor.combat_rating());
        for(auto const &[name,actual]:{std::pair{"internal",std::span<int16_t const>{damage.internal}}, {"armour",std::span<int16_t const>{damage.armour}}, {"present",std::span<int16_t const>{damage.present}}}) {
          auto const expected{words(file)};check(std::string(name)+" size",expected.size(),actual.size());
          for(size_t j{0};j!=expected.size();++j)check(std::string(name)+'['+std::to_string(j)+']',expected[j],actual[j]);
        }
        auto const expected_cells{words(file)};auto const actual_cells{body.model().cell_frames()};
        check("cell count",expected_cells.size(),actual_cells.size());
        for(size_t j{0};j!=expected_cells.size();++j)check("cell "+std::to_string(j),static_cast<uint16_t>(expected_cells[j]),actual_cells[j]);
        ++states;
      }
      for(size_t i{0};i!=nf;++i) {
        vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
        vec3<int16_t> const velocity{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        vec3<int16_t> const rates{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        auto const strength{file.integer<int16_t>()},timer{file.integer<int16_t>()};bool const obstacle{file.integer<uint8_t>()!=0};
        auto const type{file.integer<uint16_t>()};auto const clock{file.integer<int32_t>()};bool const detectable{file.integer<uint8_t>()!=0};
        auto const label{prefix+" flyer "+std::to_string(i)};
        auto const &flight{flyers[i].actor.motion()};auto const &controller{session.activity_state().decisions()[nh+i].controller};
        if(position!=flight.position || angles!=flight.angles || velocity!=flight.velocity || rates!=flight.angular_step)file.fail(label+" motion");
        if(strength!=flight.obstacle_strength || timer!=flight.obstacle_timer || obstacle!=flight.obstacle || detectable!=flyers[i].actor.is_detectable())file.fail(label+" avoidance/sensor");
        if(!controller.type || std::to_underlying(*controller.type)!=type || controller.reconsider_remaining!=clock)file.fail(label+" controller");
        ++states;
      }
      if(!matches) {
        std::cerr<<"Contacts:";
        for(auto const &hit:session.contacts())std::cerr<<' '<<hit.other<<'/'<<hit.damage;
        std::cerr<<'\n';file.fail(prefix+" state mismatch");
      }
      auto const expected{random_input(file)};auto const &actual{session.random_snapshot()};
      if(actual.words!=expected.words || actual.first!=expected.first || actual.second!=expected.second)file.fail(prefix+" RNG");
      if(extended) {
        auto const check{[&](char const *field,int actual_value){auto const expected_value{file.integer<int32_t>()};if(actual_value!=expected_value){std::cerr<<prefix<<' '<<field<<": expected "<<expected_value<<", actual "<<actual_value<<'\n';file.fail(field);}}};
        check("outcome status",std::to_underlying(session.mission_state(true)));check("debrief grade",session.result_grade());
        check("prompt",std::to_underlying(session.take_mission_prompt().value_or(mission_status::none)));
        auto const &clock{session.mission_clock()};check("poll clock",clock.poll_remaining);check("confirmation clock",clock.confirm_remaining);check("pending",clock.pending);check("reported",std::to_underlying(clock.reported));
        auto const &progress{session.objective_progress()};check("arrival",progress.arrived);check("rescue",progress.rescued);check("announced",progress.target_announced);check("rescue stage",progress.rescue_stage);check("rescue clock",progress.rescue_remaining);
      }
      if(authored){
        auto const check{[&](std::string const &label,auto wanted,auto found){if(wanted!=found){
          std::cerr<<prefix<<' '<<label<<": expected "<<wanted<<", actual "<<found<<'\n';file.fail(label);
        }}};
        auto const &activity{session.activity_state()};
        for(size_t i{};i!=100;++i)check("counter "+std::to_string(i),file.integer<int16_t>(),session.mission_variables()[i]);
        check("controller stagger",file.integer<uint16_t>(),activity.entry_stagger());
        auto triggers=words(file);check("trigger count",triggers.size(),script.triggers.size());
        for(size_t i{};i!=triggers.size();++i)check("trigger "+std::to_string(i),triggers[i]!=0,activity.triggered(i));
        check("group count",file.integer<uint16_t>(),activity.all_groups().size());
        for(size_t i{};i!=activity.all_groups().size();++i){
          check("route "+std::to_string(i),file.integer<int16_t>(),activity.group(i).route_cursor);
          check("waiting "+std::to_string(i),file.integer<uint8_t>()!=0,activity.group(i).activation_trigger.has_value());
        }
        auto targets=words(file);check("target count",targets.size(),activity.size());
        for(size_t i{};i!=targets.size();++i)check("target "+std::to_string(i),targets[i],
          activity.targets()[i].selected?static_cast<int>(*activity.targets()[i].selected):-1);
        auto const bases=session.ground_actors();check("base count",file.integer<uint16_t>(),bases.size());
        for(size_t i{};i!=bases.size();++i){
          auto const &base=bases[i].actor;auto label="base "+std::to_string(i);
          for(unsigned axis{};axis!=3;++axis)check(label+" position",file.integer<int32_t>(),base.placement().translation[axis]);
          check(label+" damage",file.integer<int16_t>(),base.damage());check(label+" dead",file.integer<uint8_t>()!=0,base.dead());
          auto cells=words(file);auto const actual_cells=base.instance().cell_frames();check(label+" cell count",cells.size(),actual_cells.size());
          for(size_t j{};j!=cells.size();++j)check(label+" cell",static_cast<uint16_t>(cells[j]),actual_cells[j]);
        }
      }
      if(player_input_frames) {
        auto const check{[&](std::string const &field,auto expected_value,auto actual_value){if(actual_value!=expected_value){std::cerr<<prefix<<' '<<field<<": expected "<<+expected_value<<", actual "<<+actual_value<<'\n';file.fail(field);}}};
        auto const &controls{session.player_weapon_controls()};auto const &sensor{session.player_sensors()};
        check("weapon selection",file.integer<int16_t>(),controls.selection());
        check("weapon group",file.integer<uint8_t>(),controls.current_group());
        check("hold selection",file.integer<uint8_t>()!=0,controls.holds_selection());
        check("auto tracking",file.integer<uint8_t>()!=0,controls.auto_tracking());
        check("manual fired",file.integer<uint8_t>()!=0,controls.manual_launched());
        check("manual guidance",file.integer<uint8_t>()!=0,controls.manual_guidance());
        auto const player_id{std::ranges::find_if(hercs,[](auto const &h){return h.player;})-hercs.begin()};
        auto const ordinal{[](std::optional<size_t> value){return value ? static_cast<int32_t>(*value) : -1;}};
        check("simulation target",file.integer<int32_t>(),ordinal(session.activity_state().targets()[static_cast<size_t>(player_id)].selected));
        check("cockpit target",file.integer<int32_t>(),ordinal(session.target_selection()));
        for(size_t i{0};i!=5;++i)check("lock "+std::to_string(i),file.integer<uint8_t>()!=0,sensor.locks[i]);
        check("HUD lock",file.integer<uint8_t>()!=0,sensor.hud_locked);
        auto const slots{session.player_actor().weapons().slots()};
        check("weapon count",file.integer<uint16_t>(),slots.size());
        for(size_t i{0};i!=slots.size();++i) {
          auto const id{file.integer<int16_t>()};check("weapon identity",id,slots[i]?slots[i]->definition:-1);
          if(id<0)continue;
          auto const &weapon{*slots[i]};auto const label{"weapon "+std::to_string(i)};
          check(label+" refire",file.integer<int16_t>(),weapon.controls().refire_remaining);
          check(label+" disabled",file.integer<uint8_t>()!=0,weapon.controls().disabled);
          check(label+" paired",file.integer<uint8_t>()!=0,controls.linked(i));
          check(label+" amount",file.integer<int16_t>(),weapon.energy()?weapon.energy()->charge:weapon.ammunition()?weapon.ammunition()->rounds:-1);
          check(label+" charge setting",file.integer<int16_t>(),weapon.energy()?weapon.energy()->setting:-1);
          for(unsigned int axis{0};axis!=3;++axis)check(label+" convergence",file.integer<int16_t>(),weapon.convergence[axis]);
        }
      }
      ++frames;
    }
  }
  file.require_end();if(frames!=expected_frames)file.fail("frame coverage");
  std::cout<<frames<<" original world frames match, including "<<states<<" HERC/flyer states\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
