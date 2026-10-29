#include "shell_statistics.h"
#include "resources/packed_strings.h"
#include "shell/game_state.h"
namespace earthsiege {
shell_statistics::shell_statistics(resource_store const &r)
 :values{read_font(r.read("shell0","dfn/cstats_1.dfn"))},labels{read_font(r.read("shell0","dfn/cstats_2.dfn"))},
 strings{read_packed_strings(r.read("shell0","eng/watext.bin"))}{}
void shell_statistics::career(framebuffer &f,rectangle const &b,career_state const &s,std::string const &name)const{
 f.fill(b,31);
 auto label{[&](size_t id,int x,int y){f.text(labels,strings.at(id),b.left+x,b.top+y);}};
 auto value{[&](std::string const &text,int x,int y){f.text(values,text,b.left+x,b.top+y);}};
 auto const &pilot{s.crew.at(static_cast<size_t>(s.selected_crew))};
 label(167,8,9);value(name,36,9);
 label(169,8,18);value(std::to_string(int{s.step}+1),81,18);
 label(170,8,27);value(std::to_string(s.missions),81,27);
 label(171,8,36);value(std::to_string(s.budget)+" KG",81,36);
 label(172,8,45);label(173,46,45);value(std::to_string(pilot.total[0]),81,45);
 label(175,46,54);value(std::to_string(pilot.total[2]),81,54);
 label(174,106,45);value(std::to_string(pilot.total[1]),161,45);
 label(176,106,54);value("0",161,54);
 label(168,113,9);value(strings.at(pilot.health?182:183),147,9);
}
void shell_statistics::debrief(framebuffer &f,rectangle const &b,career_state const &s,return_presentation const &r)const{
 f.fill(b,31);
 auto label{[&](size_t id,int x,int y){f.text(labels,strings.at(id),b.left+x,b.top+y);}};
 auto value{[&](int number,int x,int y){f.text(values,std::to_string(number),b.left+x,b.top+y);}};
 label(171,5,8);label(172,5,17);label(173,14,26);label(175,14,35);label(174,14,44);
 label(179,5,53);label(180,5,62);label(181,5,71);label(177,83,17);label(178,118,17);
 value(r.recovery,89,8);
 constexpr std::array<size_t,3> order{0,2,1};
 for(size_t i{};i<order.size();++i){int const y{26+9*static_cast<int>(i)};
  value(s.crew[0].current[order[i]],94,y);value(r.statistics[order[i]],127,y);
 }
 value(r.deaths,94,53);value(r.recovered_weapons,127,62);
 auto const technology{r.technology?strings.at(static_cast<size_t>((s.technology.last_type?213:27)+s.technology.last_index)):std::string{"NONE"}};
 f.text(values,technology,b.left+(b.right-b.left-values.text_width(technology))/2,b.top+81);
}
}
