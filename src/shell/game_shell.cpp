#include "game_shell.h"
#include "shell/launch_checks.h"
#include "resources/packed_strings.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
namespace earthsiege {
namespace {
mission_options options(preferences const &p){auto const &v{p.current()};return {v[12],v[13],v[14],v[15],v[16]};}
unsigned region(unsigned screen){
  if(screen==1500||screen==1501)return 6000;
  if(screen==1510||screen==1520)return 5000;
  if(screen==1700||screen==1710||screen==1800||screen==1900||screen==2000||screen==1910||screen==1920||screen==4444||screen==1610)return 4000;
  return 1000;
}
}
game_shell::game_shell(resource_store const &r,std::filesystem::path root,int seed)
 :resources{r},saves{std::move(root)},careers{saves.index()},preferences_{saves.load_preferences()},audio{r},game{r,seed},
 strings{read_packed_strings(r.read("shell0","eng/watext.bin"))},text_face{read_font(r.read("shell0","dfn/nm_green.dfn"))},prices{read_service_prices(r.read("shell0","cfg/damage.dat"))},build_view{r},armament{r},bay{r},crew_view{r}{show(1000);}
void game_shell::start_speech(int mode,int ordinal){
 speech.reset();
 speech_mode=mode;speech_ordinal=ordinal;
 auto const &header{game.prepared().compiled.script.header};
 auto selected{select_shell_speech(mode,game.crew().node,mode==2?ordinal:header.objective_variant,
   game.prepared().counters[0],header.objective_kind,game.crew().missions!=0,game.presentation_random())};
 speech_destination=selected.destination;
 speech_screen=mode==2?1490:mode<=1?1499:mode==6?1700:selected.destination;
 framebuffer base;
 if(selected.region==4000){
  if(!bay_motion)show(1700);
  base=bay_motion->image();
 }else{
  if(bay_motion)audio.clear_sample();
  bay_motion.reset();weapon_motion.reset();preview_index=-2;displayed_hangar.clear();
  base.image(read_bitmaps(resources.read("shell0","res32/regn"+std::to_string(selected.region)+".dbm")).front(),0,0);
 }
 speech_palette=read_palette(resources.read("shell0","res32/regn"+std::to_string(selected.region)+".dpl"));
 audio.region(selected.region);if(selected.song>=0)audio.song(selected.song);
 speech=std::make_unique<shell_speech_scene>(resources,selected,base,game.presentation_random());
 audio.speak(speech->sample(),preferences_.current()[2]!=0);
}
void game_shell::finish_speech(bool skipped){
 if(!speech)return;
 auto const mode{speech_mode};auto const destination{speech_destination};auto const ordinal{speech_ordinal};
 speech->stop(game.presentation_random());speech.reset();speech_mode=-1;audio.stop_speech();
 if(mode==2&&!skipped&&static_cast<size_t>(ordinal+1)<chapter_introduction(game.crew().node).size()){
  start_speech(2,ordinal+1);return;
 }
 if(mode==6){launch_requested=true;save_career();return;}
 if(destination==1490){start_speech(2);return;}
 show(destination);
}
void game_shell::show(unsigned id){
  if(menu)last_screen=menu->id();
  menu=std::make_unique<dialog>(resources,id,region(id));text_scroll=0;
  audio.enabled(preferences_.current()[0]!=0,preferences_.current()[1]!=0);audio.region(region(id));
  if((id==1500||id==1501)&&game.ready()&&!briefing){
    std::vector<std::string> text;
    if(auto section{game.text().find(mission_text_kind::objectives)})
      if(auto objectives{std::get_if<std::vector<objective_text>>(&section->content)})
        for(auto const &objective:*objectives)text.push_back(objective.text);
    briefing=std::make_unique<briefing_map>(resources,game.prepared().compiled.script,static_cast<int>(game.player().hercs.size()),std::move(text));
    briefing->start_tour(true);
  }else if((id==1500||id==1501)&&briefing)briefing->restore_view();
  if(id==1000){menu->enable(1700,!careers.entries.empty());std::vector<std::string> names;for(auto const &entry:careers.entries)names.push_back(entry.name);menu->items(1002,std::move(names),std::max(0,int(careers.selected)));}
  if(id==1100){std::vector<std::string> labels;for(size_t i{35};i<=41;++i)labels.push_back(strings.at(i));menu->items(1101,std::move(labels),single_kind);}
  if(id==1110||id==1310){preferences_.commit(false);refresh_values();}
  if(id==1200){std::vector<std::string> names;for(auto const &e:careers.entries)names.push_back(e.name);menu->items(1202,std::move(names),std::max(0,int(careers.selected)));
    for(int action: {1201,1220,1230,1700})menu->enable(static_cast<uint16_t>(action),!careers.entries.empty());}
  if(id==1330){menu->items(1331,{strings.at(19),strings.at(20),strings.at(21)},battle_set);}
  if(id==1610){crew_edit.emplace(game.roster(),game.crew());menu->enable(1623,false);}
  if(id==1800){selected_mount=-1;missile_menu=false;refresh_armament();}
  if(id==1900||id==1920)refresh_repair();
  if(id==2000){build_menu.emplace(game.roster(),game.crew(),prices,build_chassis);refresh_construction();}
  if(id==1910)for(uint16_t i{};i<3;++i)menu->items(static_cast<uint16_t>(1914+i),{"",""},game.crew().repair_policy==i);
  if(id==4444)if(auto h{game.roster().find(game.roster().selected)})menu->replace_text("###",std::to_string(chassis_recovery(*h,prices)/1000));
  if(id==2000&&bay_motion)bay_motion->construction(game.roster(),prices,game.presentation_random());
  if(id==1220){history_slots.clear();std::vector<std::string> labels;auto const &states{game.career().career.states};
    for(size_t i{1};i<states.size();++i)if(states[i].valid){history_slots.push_back(i);labels.push_back("MISSION "+std::to_string(states[i].missions));}
    menu->items(1223,std::move(labels));menu->enable(1222,!history_slots.empty());menu->enable(1230,!history_slots.empty());}
  refresh_bay();refresh_preview();
}
void game_shell::refresh_bay(){
 if(warning_parent||speech)return;
 if(region(screen())!=4000){if(bay_motion)audio.clear_sample();bay_motion.reset();displayed_hangar.clear();return;}
 if(!bay_motion){
  bay_backdrop.clear();bay_backdrop.image(read_bitmaps(resources.read("shell0","res32/regn4000.dbm")).front(),0,0);
  bay_motion=std::make_unique<bay_scene>(resources);
  bay_motion->reset(game.roster(),game.crew(),game.campaign(),game.mission_kind(),prices,game.presentation_random(),bay_backdrop,audio.effects());
  displayed_hangar.clear();displayed_mount=-2;
 }
 auto current{write_hangar(game.roster())};auto const mount{screen()==1800?selected_mount:-1};
 if(current!=displayed_hangar||mount!=displayed_mount){
  bay_motion->redraw(game.roster(),bay,armament,mount,bay_backdrop,game.presentation_random(),audio.effects());
  displayed_hangar=std::move(current);displayed_mount=mount;
 }
}
void game_shell::advance(unsigned timer_ticks){
 audio.enabled(preferences_.current()[0]!=0,preferences_.current()[1]!=0);
 for(unsigned i{};i<timer_ticks;++i){
  if((++animation_ticks%2)==0&&!warning_parent){
   if(speech){speech->advance(game.presentation_random());if(speech->finished())finish_speech(false);}
   else {
   if(bay_motion)bay_motion->advance(1,game.presentation_random(),audio.effects());
   if(weapon_motion)weapon_motion->advance(1,game.presentation_random());
   if(briefing&&(screen()==1500||screen()==1501))briefing->update(static_cast<uint32_t>(animation_ticks));
   }
  }
  audio.advance();
 }
}

void game_shell::refresh_preview(){
 if(warning_parent||speech)return;
 int index{-1};
 if(screen()==1800){
  auto const selected{menu->selection(1808)};
  if(missile_menu)index=14+selected;
  else if(selected>=0&&static_cast<size_t>(selected)<weapon_choices.size())
   index=weapon_choices[static_cast<size_t>(selected)]==17?-1:weapon_choices[static_cast<size_t>(selected)];
 }
 if(index==preview_index)return;
 preview_index=index;weapon_motion.reset();
 if(index>=0){framebuffer base;menu->draw(base);weapon_motion=std::make_unique<weapon_preview>(resources,index,base,game.presentation_random());}
}

void game_shell::refresh_armament(std::optional<int16_t> wanted){
  auto const h{game.roster().find(game.roster().selected)};
  if(!h){menu->items(1808,{});menu->enable(1111,false);return;}
  menu->items(1903,{strings.at(static_cast<size_t>(27+h->chassis))});
  std::string pilot;for(size_t i{};i<game.crew().crew.size();++i)if(game.crew().crew[i].hangar_slot==h->slot){
    auto const &c{game.crew().crew[i]};pilot=strings.at(i?static_cast<size_t>(86+c.name*3+c.variant):85);break;
  }
  menu->items(1904,{pilot});
  if(missile_menu){
    menu->items(1808,{strings.at(64),strings.at(65),strings.at(66),strings.at(68)},h->weapon_values.at(static_cast<size_t>(selected_mount)));return;
  }
  weapon_choices=armament.choices(*h,selected_mount,game.crew().technology,game.inventory());
  std::vector<std::string> labels;
  for(auto weapon:weapon_choices){
    size_t const key{weapon==17?45:weapon>=14?static_cast<size_t>(46+weapon):static_cast<size_t>((game.campaign()?46:71)+weapon)};
    auto label{strings.at(key)};
    if(game.campaign()&&weapon!=17&&label.starts_with("[??]")){
      auto count{std::to_string(game.inventory().at(static_cast<size_t>(weapon)).size())};if(count.size()<2)count.insert(0," ");
      label.replace(1,2,count.substr(0,2));
    }
    labels.push_back(std::move(label));
  }
  auto desired{wanted.value_or(selected_mount<0?int16_t{0}:h->weapons.at(static_cast<size_t>(selected_mount)))};
  auto found{std::ranges::find(weapon_choices,desired)};
  int const index{found==weapon_choices.end()?0:static_cast<int>(found-weapon_choices.begin())};
  menu->items(1808,std::move(labels),index);
  auto mounted{selected_mount<0?int16_t{17}:h->weapons.at(static_cast<size_t>(selected_mount))};
  menu->enable(1111,mounted>=9&&mounted<=11);
}
void game_shell::choose_armament(){
  auto h{game.roster().find(game.roster().selected)};if(!h)return;
  auto choice{menu->selection(1808)};
  if(missile_menu){select_missile_type(*h,selected_mount,static_cast<int16_t>(choice));return;}
  if(choice<0||static_cast<size_t>(choice)>=weapon_choices.size())return;
  auto const weapon{weapon_choices[static_cast<size_t>(choice)]};
  if(selected_mount>=0)armament.exchange(*h,selected_mount,weapon,game.inventory());
  refresh_armament(weapon);
}

void game_shell::refresh_construction(){
 if(!build_menu)return;
 menu->enable(2001,build_menu->enabled());menu->enable(2003,build_menu->cycling());menu->enable(2004,build_menu->cycling());
 menu->find(2001)->label=strings.at(build_menu->building(game.roster())?205:206);
}
void game_shell::refresh_repair(){
 auto h{game.roster().find(game.roster().selected)};
 if(screen()==1900){
  for(auto id:{1920,1901,1902})menu->enable(static_cast<uint16_t>(id),h!=nullptr);
  if(!h){for(auto id:{1903,1904,1905,1906})menu->items(static_cast<uint16_t>(id),{});menu->items(1907,{std::to_string(game.crew().budget)});return;}
  auto &crew{game.crew()};crew.crew_cursor=-1;
  std::string pilot{strings.at(122)};
  for(size_t i{};i<crew.crew.size();++i)if(crew.crew[i].hangar_slot==h->slot){
   crew.crew_cursor=static_cast<int16_t>(i);
   auto const &c{crew.crew[i]};
   pilot=i?strings.at(static_cast<size_t>(86+c.name*3+c.variant)):careers.selected>=0?careers.entries.at(static_cast<size_t>(careers.selected)).name:strings.at(85);
   break;
  }
  menu->items(1903,{strings.at(static_cast<size_t>(27+h->chassis))});
  menu->items(1904,{pilot});menu->items(1905,{strings.at(static_cast<size_t>(123+condition_tier(herc_average_condition(*h))))});
  menu->items(1906,{std::to_string(herc_average_condition(*h)<100?repair_quote(*h,100,prices):0)});
  menu->items(1907,{std::to_string(crew.budget)});return;
 }
 if(screen()==1920&&h&&repair_edit){
  auto const group{repair_edit->group()};std::vector<std::string> labels;
  for(size_t i{};i<component_count(*h,group);++i){
   size_t const key{group==repair_group::internal?128+i:group==repair_group::armour?137+i:static_cast<size_t>(143+h->weapons.at(i))};
   auto label{strings.at(key)};
   if(!label.starts_with("-")){auto value{std::to_string(component_condition(h->condition,group,static_cast<int16_t>(i)))};if(value.size()<3)value.insert(0,3-value.size(),' ');label.replace(0,3,value);}
   labels.push_back(std::move(label));
  }
  menu->items(1921,std::move(labels),repair_edit->index());
  menu->items(1922,{strings.at(static_cast<size_t>(123+repair_edit->tier()))});
  menu->items(1907,{std::to_string(game.crew().budget)});menu->items(1927,{std::to_string(repair_edit->required())});
  menu->items(1923,{std::to_string(repair_edit->allocated())});menu->enable(106,repair_edit->can_increase(game.crew()));
 }
}

void game_shell::refresh_values(){
  auto const list{[&](uint16_t action,std::initializer_list<size_t> indices,int selected){std::vector<std::string> labels;for(auto i:indices)labels.push_back(strings.at(i));menu->items(action,std::move(labels),selected);}};
  auto const &v{preferences_.current()};
  if(screen()==1110){list(1116,{8,9},v[12]);list(1117,{10,11},v[13]);list(1118,{23,24,25,26},v[14]);list(1119,{12,4},v[15]);list(1120,{13,14,15},v[16]);}
  if(screen()==1310){list(1314,{2,3},v[0]);list(1315,{2,3},v[1]);list(1318,{16,17,18},v[2]);list(1316,{4,5,6,7},(v[3]+v[4]+v[5]+v[6])/256);}
}
void game_shell::save_career(){
  if(!game.campaign()||!game.has_career()||careers.selected<0)return;
  game.checkpoint();saves.save(careers.entries.at(static_cast<size_t>(careers.selected)),game.career());saves.save_index(careers);
}
void game_shell::select_career(){
  if(careers.entries.empty())return;
  auto const &entry{careers.entries.at(static_cast<size_t>(careers.selected))};game.load_campaign(saves.load(entry),entry.portrait);
  saves.save_index(careers);
}
void game_shell::register_career(){
  auto name{menu->find(103)->label};if(name.empty())name=strings.at(22);
  auto it{std::ranges::find_if(careers.entries,[&](auto const &e){return e.name==name && e.portrait==0;})};
  if(it==careers.entries.end()){
    size_t suffix{};std::string car,mec;
    do{car="car\\car_"+std::to_string(suffix)+".car";mec="car\\car_"+std::to_string(suffix)+".mec";++suffix;}while(saves.read(car)||saves.read(mec));
    careers.entries.push_back({name,0,car,mec});careers.selected=static_cast<int16_t>(careers.entries.size()-1);
  }else careers.selected=static_cast<int16_t>(it-careers.entries.begin());
  game.new_campaign(static_cast<int16_t>(battle_set));briefing.reset();save_career();start_speech(2);
}
void game_shell::warn(unsigned id){
  draw(warning_frame);warning_parent=std::move(menu);
  menu=std::make_unique<dialog>(resources,id,region(warning_parent->id()));
}
void game_shell::dismiss_warning(){menu=std::move(warning_parent);}
void game_shell::next_mission(){
  game.prepare_campaign(true,options(preferences_));briefing.reset();save_career();show(game.ready()?1700:1000);
}
void game_shell::begin(){start_speech(6);save_career();}
std::unique_ptr<mission_session> game_shell::launch(){
  if(!launch_requested)throw std::runtime_error{"no requested launch"};
  launch_requested=false;audio.suspend();auto result{game.launch()};save_career();return result;
}
void game_shell::mission_return(mission_results const &result,mission_counters const &counters,int16_t return_seed){
  game.resume_shell(return_seed,game.campaign()?std::optional{saves.load(careers.entries.at(static_cast<size_t>(careers.selected)))}:std::nullopt);
  game.complete(result,counters);briefing.reset();save_career();audio.resume();if(game.campaign())show(1520);else start_speech(3);
}
void game_shell::action(uint16_t id){if(speech){finish_speech(true);return;}action_impl(id);refresh_bay();refresh_preview();}
void game_shell::action_impl(uint16_t id){
  auto const current{screen()};
  switch(current){
    case 3333:case 7770:case 7772:
      if(id==99)dismiss_warning();
      return;
    case 3000:
      if(id==99)dismiss_warning();
      else if(id==100){dismiss_warning();begin();}
      return;
    case 7771:
      if(id==99)dismiss_warning();
      else if(id==1499){dismiss_warning();next_mission();}
      return;
    case 999:if(id==100)quitting=true;else if(id==99)show(1000);return;
    case 1000:
      if(id==1003){show(999);return;}
      if(id==1499){game.prepare_single(-1,options(preferences_));briefing.reset();start_speech(0);return;}
      if(id==1700){select_career();if(game.has_career()){if(game.crew().screen==1710)show(1710);else{game.prepare_campaign(false,options(preferences_));briefing.reset();show(1700);}}return;}
      if(id==1002){careers.selected=static_cast<int16_t>(menu->selection(1002));return;}
      break;
    case 1100:
      if(id==1101){single_kind=menu->selection(1101);return;}
      if(id==101||id==102){menu->select(1101,single_kind+(id==101?-1:1));single_kind=menu->selection(1101);return;}
      if(id==1501){game.prepare_single(static_cast<int16_t>(single_kind),options(preferences_));briefing.reset();start_speech(0);return;}
      break;
    case 1110:
      if(id>=1111&&id<=1115){constexpr std::array<uint8_t,5> limits{2,2,4,2,3};size_t const i{id-1111U};preferences_.set(12+i,static_cast<uint8_t>((preferences_.current()[12+i]+1)%limits[i]),false);refresh_values();return;}
      if(id==99||id==100){if(id==100){preferences_.commit(false);saves.save_preferences(preferences_);}else preferences_.cancel(false);show(1100);return;}
      return;
    case 1300:
      if(id==100){show(options_owner);return;}
      break;
    case 1310:
      if(id==1311||id==1312)preferences_.toggle(id==1311?0:1);
      else if(id==1317)preferences_.set(2,static_cast<uint8_t>((preferences_.current()[2]+1)%3));
      else if(id==1313){auto const &v{preferences_.current()};int const detail{((v[3]+v[4]+v[5]+v[6])/256+1)%4};preferences_.set(3,detail?1:0);for(size_t i{4};i<=6;++i)preferences_.set(i,static_cast<uint8_t>(detail*64+63));}
      else if(id==99||id==100){if(id==100){preferences_.commit();saves.save_preferences(preferences_);}else preferences_.cancel();show(1300);return;}
      refresh_values();return;
    case 1330:
      if(id==101||id==102){menu->select(1331,menu->selection(1331)+(id==101?-1:1));return;}
      if(id==100){battle_set=menu->selection(1331);show(1300);return;}
      if(id==1331)return;
      break;
    case 1200:
      if(id==1202){careers.selected=static_cast<int16_t>(menu->selection(1202));return;}
      if(id==101||id==102){menu->select(1202,int(careers.selected)+(id==101?-1:1));careers.selected=static_cast<int16_t>(menu->selection(1202));return;}
      if(id==1201 && careers.selected>=0){auto const i{static_cast<size_t>(careers.selected)};saves.erase(careers.entries.at(i));careers.entries.erase(careers.entries.begin()+static_cast<ptrdiff_t>(i));careers.selected=careers.entries.empty()?-1:static_cast<int16_t>(i?i-1:0);saves.save_index(careers);show(1200);return;}
      if(id==1700||id==1220||id==1230){select_career();if(game.crew().crew[0].health==0){show(1250);return;}
        if(id==1700){if(game.crew().screen==1710)id=1710;else game.prepare_campaign(false,options(preferences_));}show(id);return;}
      break;
    case 1210:if(id==100)register_career();else if(id==99)show(1200);return;
    case 1250:if(id==99)show(1200);return;
    case 1230:if(id==99)show(last_screen==1220?1220:1200);return;
    case 1220:
      if(id==101||id==102){menu->select(1223,menu->selection(1223)+(id==101?-1:1));return;}
      if(id==1221){game.checkpoint(std::min<size_t>(40,static_cast<size_t>(game.crew().missions)+1));save_career();show(1220);return;}
      if(id==1222&&!history_slots.empty()){game.restore_history(history_slots.at(static_cast<size_t>(menu->selection(1223))));save_career();show(1200);return;}
      if(id==1223)return;
      break;
    case 1700:case 1500:case 1501:
      if(current==1700&&id==1500&&!game.crew().current){start_speech(1);return;}
      if(briefing&&(current==1500||current==1501)){
        if(id==108||id==109||(id>=1503&&id<=1506)){briefing->navigate(id);return;}
        if(id==1700)briefing->skip_tour();
      }
      if(id==100){if(auto warning{launch_warning(game.roster(),game.crew())})warn(warning);else begin();return;}
      if(id==99){show(game.campaign()?1700:1100);return;}
      if(id==101||id==102){text_scroll=std::max(0,text_scroll+(id==101?-1:1));return;}
      if(id==1800){show(1800);return;}
      break;
    case 1710:
      if(id==1499){if(auto warning{next_mission_warning(game.roster(),game.crew())})warn(warning);else next_mission();return;}
      if(id==2000&&!construction_available(game.roster())){warn(7772);return;}
      break;
    case 1610:
      if(id==100){save_career();crew_edit->accept(game.roster(),game.crew());crew_edit.reset();show(1700);return;}
      if(id==1700){crew_edit->cancel(game.roster(),game.crew());crew_edit.reset();show(1700);return;}
      if(id==2003||id==2004)crew_edit->cycle(game.roster(),game.crew(),id==2004?1:-1);
      else if(id==1623&&menu->find(1623)->enabled)crew_edit->none(game.roster(),game.crew());
      else if(id==1624)crew_edit->select(game.roster(),game.crew(),static_cast<int16_t>((crew_edit->selected_pilot()+1)%4));
      else if(id>=1615&&id<=1618){auto member{static_cast<int16_t>(id-1615)};
        if(member==crew_edit->selected_pilot())crew_edit->cycle(game.roster(),game.crew(),1);
        else crew_edit->select(game.roster(),game.crew(),member);
      }
      menu->enable(1623,crew_edit->selected_pilot()!=0);return;
    case 1800:
      if(id==100){save_career();show(game.campaign()?1700:1501);return;}
      if(id==1808){choose_armament();return;}
      if(id==1801||id==1802){menu->select(1808,menu->selection(1808)+(id==1801?-1:1));choose_armament();return;}
      if(id==1806||id==1807){
        auto h{game.roster().find(game.roster().selected)};if(!h||h->weapons.empty())return;
        auto count{static_cast<int>(h->weapons.size())};selected_mount+=id==1807?1:-1;
        if(selected_mount<0)selected_mount=count-1;
        selected_mount%=count;missile_menu=false;refresh_armament();return;
      }
      if(id==1111&&menu->find(1111)->enabled){missile_menu=!missile_menu;refresh_armament();return;}
      if(id==2003||id==2004){
        cycle_arming_herc(game.roster(),game.crew(),id==2004?1:-1,!game.campaign());
        selected_mount=-1;missile_menu=false;refresh_armament();return;
      }
      return;

    case 2000:
      if(id==1710){save_career();build_chassis=build_menu->chassis();build_menu.reset();show(1710);return;}
      if(id==2001){
        bool const cancelling{build_menu->building(game.roster())};
        build_menu->activate(game.roster(),game.crew(),game.inventory(),armament);
        if(cancelling&&bay_motion)bay_motion->construction(game.roster(),prices,game.presentation_random());
      }
      else if(id==2003||id==2004)build_menu->cycle(game.crew(),id==2004?1:-1);
      refresh_construction();return;
    case 1900:
      if(id==1710){save_career();show(1710);return;}
      if(id==2003||id==2004){cycle_arming_herc(game.roster(),game.crew(),id==2004?1:-1,false);refresh_repair();return;}
      if(id==1910){show(1910);return;}
      if(auto h{game.roster().find(game.roster().selected)}){
        if(id==1920){repair_edit.emplace(*h,game.crew(),prices);show(1920);return;}
        if(id==1901){show(4444);return;}
        if(id==1902){auto result{repair_herc(*h,game.crew().budget,prices)};if(result.applied){
          game.crew().budget-=result.cost;auto const cursor{game.crew().crew_cursor};
          if(cursor>=0&&cursor<4)game.crew().crew[static_cast<size_t>(cursor)].budget_share=game.crew().budget;
        }normalize_hangar_equipment(*h);refresh_repair();return;}
      }
      return;
    case 1910:
      if(id>=1911&&id<=1913){game.crew().repair_policy=static_cast<int16_t>(id-1911);show(1900);}
      return;
    case 1920:
      if(auto h{game.roster().find(game.roster().selected)};h&&repair_edit){
        if(id==99||id==1900){
          if(id==99)repair_edit->cancel(*h,game.crew());else{repair_edit->accept(*h,game.crew());normalize_hangar_equipment(*h);}
          repair_edit.reset();show(1900);return;
        }
        if(id>=1924&&id<=1926)repair_edit->select(*h,static_cast<repair_group>(id),0);
        else if(id==1921)repair_edit->select(*h,repair_edit->group(),static_cast<int16_t>(menu->selection(1921)));
        else if(id==101||id==102){
          menu->select(1921,menu->selection(1921)+(id==101?-1:1));
          repair_edit->select(*h,repair_edit->group(),static_cast<int16_t>(menu->selection(1921)));
        }else if(id==106)repair_edit->increase(*h,game.crew());else if(id==107)repair_edit->decrease(*h,game.crew());
        refresh_repair();
      }
      return;
    case 4444:
      if(id==100)scrap_selected(game.roster(),game.crew(),game.inventory(),prices);
      if(id==100||id==1900)show(1900);
      return;

    case 1510:case 1520:
      if(id==101||id==102){text_scroll=std::max(0,text_scroll+(id==101?-1:1));return;}
      break;
    default:break;
  }
  if(id==1300)options_owner=current;
  if(id==2000||id==1900||id==1610||id==1000||id==1100||id==1110||id==1200||id==1210||id==1220||id==1230||id==1300||id==1310||id==1330||id==1500||id==1700||id==1710)show(id);
}
void game_shell::key(uint8_t scan){if(speech){if(scan)finish_speech(true);return;}if(scan==14){menu->backspace();return;}if(auto id{menu->shortcut(scan)})action(*id);}
void game_shell::mouse_move(int x,int y){if(speech)return;if(briefing&&(screen()==1500||screen()==1501))briefing->hover(x,y);}
void game_shell::mouse_down(int x,int y){
  if(speech){finish_speech(true);consume_release=true;return;}
  mouse_move(x,y);
  pressed_mount.reset();
  if(screen()==1800)if(auto h{game.roster().find(game.roster().selected)})pressed_mount=armament.hit(h->chassis,x,y);
  if(!pressed_mount)menu->mouse_down(x,y);
}
void game_shell::mouse_up(int x,int y){
  if(consume_release){consume_release=false;return;}if(speech)return;
  auto previous{pressed_mount};pressed_mount.reset();
  if(previous&&screen()==1800){
    if(auto h{game.roster().find(game.roster().selected)})if(armament.hit(h->chassis,x,y)==previous){
      selected_mount=*previous;missile_menu=false;refresh_armament();refresh_bay();refresh_preview();
    }
    return;
  }
  if(auto id{menu->mouse_up(x,y)})action(*id);
}
void game_shell::draw_text(framebuffer &target,std::string const &value,rectangle const &bounds,int scroll)const{
  screen_mask mask;for(int y{std::max(0,bounds.top)};y<=std::min(199,bounds.bottom);++y)mask.rows[static_cast<size_t>(y)].push_back({bounds.left,bounds.right});scoped_framebuffer_clip clip{target,&mask};
  std::istringstream words{value};std::string word,line;int row{};int const step{static_cast<int>(text_face.height)};
  auto emit{[&](){int y{bounds.top+(row++-scroll)*step};target.text(text_face,line,bounds.left,y+text_face.baseline);line.clear();}};
  while(words>>word){auto next{line.empty()?word:line+" "+word};if(text_face.text_width(next)>bounds.right-bounds.left&&!line.empty()){emit();line=word;}else line=std::move(next);}if(!line.empty())emit();
}
void game_shell::draw(framebuffer &target)const{
  if(speech){speech->draw(target,preferences_.current()[2]);return;}
  if(warning_parent){target=warning_frame;menu->draw(target,false);return;}
  if(bay_motion){target=bay_motion->image();menu->draw(target,false);}else menu->draw(target);
  auto const current{screen()};
  if(current==1800||((current==1700||current==1710||current==1900||current==1920||current==4444)&&game.has_career())){

    if(current==1800){
      if(weapon_motion)if(auto view{menu->find(1803)})weapon_motion->draw(target,view->bounds);
    }
  }
  if(current==2000&&build_menu)build_view.draw(target,*build_menu,game.roster(),game.crew(),prices);
  if(current==1610&&crew_edit){
    auto const name{careers.selected>=0?careers.entries.at(static_cast<size_t>(careers.selected)).name:strings.at(85)};
    crew_view.draw(target,*menu,game.roster(),game.crew(),*crew_edit,name);
  }
  if((current==1500||current==1501||current==1510||current==1520)&&game.ready()){
    auto const kind{current==1500||current==1501?mission_text_kind::briefing:mission_text_kind::debriefing};
    if(auto section{game.text().find(kind)})if(auto program{std::get_if<text_program>(&section->content)}){
      rectangle const bounds{kind==mission_text_kind::briefing?rectangle{78,151,264,182}:rectangle{17,29,142,85}};
      draw_text(target,render_text(*program,game.prepared().counters),bounds,text_scroll);
    }
    if(kind==mission_text_kind::briefing&&briefing)briefing->draw(target);
  }
}
}
