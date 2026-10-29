#include "game_shell.h"
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
  if(screen==1700||screen==1710||screen==1800||screen==1900||screen==2000||screen==1610)return 4000;
  return 1000;
}
}
game_shell::game_shell(resource_store const &r,std::filesystem::path root,int seed)
 :resources{r},saves{std::move(root)},careers{saves.index()},preferences_{saves.load_preferences()},game{r,seed},
 strings{read_packed_strings(r.read("shell0","eng/watext.bin"))},text_face{read_font(r.read("shell0","dfn/nm_green.dfn"))}{show(1000);}
void game_shell::show(unsigned id){
  if(menu)last_screen=menu->id();
  menu=std::make_unique<dialog>(resources,id,region(id));text_scroll=0;
  if(id==1000){menu->enable(1700,!careers.entries.empty());std::vector<std::string> names;for(auto const &entry:careers.entries)names.push_back(entry.name);menu->items(1002,std::move(names),std::max(0,int(careers.selected)));}
  if(id==1100){std::vector<std::string> labels;for(size_t i{35};i<=41;++i)labels.push_back(strings.at(i));menu->items(1101,std::move(labels),single_kind);}
  if(id==1110||id==1310){preferences_.commit(false);refresh_values();}
  if(id==1200){std::vector<std::string> names;for(auto const &e:careers.entries)names.push_back(e.name);menu->items(1202,std::move(names),std::max(0,int(careers.selected)));
    for(int action: {1201,1220,1230,1700})menu->enable(static_cast<uint16_t>(action),!careers.entries.empty());}
  if(id==1330){menu->items(1331,{strings.at(19),strings.at(20),strings.at(21)},battle_set);}
  if(id==1220){history_slots.clear();std::vector<std::string> labels;auto const &states{game.career().career.states};
    for(size_t i{1};i<states.size();++i)if(states[i].valid){history_slots.push_back(i);labels.push_back("MISSION "+std::to_string(states[i].missions));}
    menu->items(1223,std::move(labels));menu->enable(1222,!history_slots.empty());menu->enable(1230,!history_slots.empty());}
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
  game.new_campaign(static_cast<int16_t>(battle_set));save_career();show(1700);
}
void game_shell::begin(){launch_requested=true;save_career();}
std::unique_ptr<mission_session> game_shell::launch(){
  if(!launch_requested)throw std::runtime_error{"no requested launch"};
  launch_requested=false;auto result{game.launch()};save_career();return result;
}
void game_shell::mission_return(mission_results const &result,mission_counters const &counters,int16_t return_seed){
  game.resume_shell(return_seed,game.campaign()?std::optional{saves.load(careers.entries.at(static_cast<size_t>(careers.selected)))}:std::nullopt);
  game.complete(result,counters);save_career();show(game.campaign()?1520:1510);
}
void game_shell::action(uint16_t id){
  auto const current{screen()};
  switch(current){
    case 999:if(id==100)quitting=true;else if(id==99)show(1000);return;
    case 1000:
      if(id==1003){show(999);return;}
      if(id==1499){game.prepare_single(-1,options(preferences_));show(1501);return;}
      if(id==1700){select_career();if(game.has_career()){if(game.crew().screen==1710)show(1710);else{game.prepare_campaign(false,options(preferences_));show(1700);}}return;}
      if(id==1002){careers.selected=static_cast<int16_t>(menu->selection(1002));return;}
      break;
    case 1100:
      if(id==1101){single_kind=menu->selection(1101);return;}
      if(id==101||id==102){menu->select(1101,single_kind+(id==101?-1:1));single_kind=menu->selection(1101);return;}
      if(id==1501){game.prepare_single(static_cast<int16_t>(single_kind),options(preferences_));show(1501);return;}
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
      if(id==100){begin();return;}
      if(id==99){show(game.campaign()?1700:1100);return;}
      if(id==101||id==102){text_scroll=std::max(0,text_scroll+(id==101?-1:1));return;}
      if(id==1800){show(1800);return;}
      break;
    case 1710:
      if(id==1499){game.prepare_campaign(true,options(preferences_));save_career();show(game.ready()?1700:1000);return;}
      break;
    case 1800:if(id==100){save_career();show(game.campaign()?1700:1501);}return;
    case 1510:case 1520:
      if(id==101||id==102){text_scroll=std::max(0,text_scroll+(id==101?-1:1));return;}
      break;
    default:break;
  }
  if(id==1300)options_owner=current;
  if(id==1000||id==1100||id==1110||id==1200||id==1210||id==1220||id==1230||id==1300||id==1310||id==1330||id==1500||id==1700||id==1710)show(id);
}
void game_shell::key(uint8_t scan){if(scan==14){menu->backspace();return;}if(auto id{menu->shortcut(scan)})action(*id);}
void game_shell::mouse_up(int x,int y){if(auto id{menu->mouse_up(x,y)})action(*id);}
void game_shell::draw_text(framebuffer &target,std::string const &value,rectangle const &bounds,int scroll)const{
  screen_mask mask;for(int y{std::max(0,bounds.top)};y<=std::min(199,bounds.bottom);++y)mask.rows[static_cast<size_t>(y)].push_back({bounds.left,bounds.right});scoped_framebuffer_clip clip{target,&mask};
  std::istringstream words{value};std::string word,line;int row{};int const step{static_cast<int>(text_face.height)};
  auto emit{[&](){int y{bounds.top+(row++-scroll)*step};target.text(text_face,line,bounds.left,y+text_face.baseline);line.clear();}};
  while(words>>word){auto next{line.empty()?word:line+" "+word};if(text_face.text_width(next)>bounds.right-bounds.left&&!line.empty()){emit();line=word;}else line=std::move(next);}if(!line.empty())emit();
}
void game_shell::draw(framebuffer &target)const{
  menu->draw(target);auto const current{screen()};
  if((current==1500||current==1501||current==1510||current==1520)&&game.ready()){
    auto const kind{current==1500||current==1501?mission_text_kind::briefing:mission_text_kind::debriefing};
    if(auto section{game.text().find(kind)})if(auto program{std::get_if<text_program>(&section->content)}){
      rectangle const bounds{kind==mission_text_kind::briefing?rectangle{18,15,278,139}:rectangle{17,29,142,85}};
      draw_text(target,render_text(*program,game.prepared().counters),bounds,text_scroll);
    }
    if(kind==mission_text_kind::briefing)if(auto section{game.text().find(mission_text_kind::objectives)})if(auto objectives{std::get_if<std::vector<objective_text>>(&section->content)}){
      std::string text;for(auto const &o:*objectives){if(!text.empty())text+=" ";text+=o.text;}draw_text(target,text,{78,151,264,182});
    }
  }
}
}
