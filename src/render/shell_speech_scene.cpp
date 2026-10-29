#include "shell_speech_scene.h"
#include "resources/packed_strings.h"
namespace earthsiege {
namespace {
shell_speech_group group(resource_store const &r,speech_selection const &s){return read_shell_speech_group(r.read("shell0",s.list_file));}
std::string caption_text(resource_store const &r,speech_selection const &s){
 auto raw{r.read("shell0",s.text_file)};binary_reader f{raw};
 if(f.integer<uint16_t>()!=2)f.fail("shell speech text revision");
 return read_packed_strings(raw.subspan(2)).at(s.entry);
}
}
void shell_speech_scene::run(animated &part,int16_t elapsed,random_state &random){
 part.player.advance(elapsed,random);part.art.apply(part.player.commands(),visible,background);
}
shell_speech_scene::shell_speech_scene(resource_store const &r,speech_selection const &s,framebuffer const &base,random_state &random,framebuffer const *retained,resident_update update)
 :external{std::move(update)},portrait{r,group(r,s).program},
 mouth{read_shell_mouth_timeline(r.read("shell0","snc/"+group(r,s).clips.at(s.entry).sync)),
       static_cast<uint16_t>(read_presentation(r.read("shell0","res32/"+group(r,s).program)).pages.size())},
 caption{r,caption_text(r,s),s.region},sample_{group(r,s).clips.at(s.entry).sample},background{retained?*retained:base},visible{base}{
 if(s.region==3000){
  auto stem{normalise_path(group(r,s).program).substr(0,2)};
  residents.emplace_back(r,stem+"_brd.ttm");residents.emplace_back(r,stem+"_brj.ttm");
 }else if(s.region==5000){
  auto stem{normalise_path(group(r,s).program).substr(0,2)};
  stem[0]='c';residents.emplace_back(r,stem+"_blnk.ttm");
 }
 for(auto &resident:residents)run(resident,0,random);
 for(auto page:{uint16_t{0},uint16_t{25},uint16_t{1}}){portrait.player.select_page(page);run(portrait,0,random);}
 portrait.player.select_page(1);
 visible=background;mouth.update(0);
 if(external)external(visible,background,0,random);
 for(auto &resident:residents)run(resident,0,random);
 portrait.player.select_page(mouth.page());run(portrait,0,random);
}
void shell_speech_scene::advance(random_state &random){
 if(finished())return;
 visible=background;mouth.update(++clock);
 if(external)external(visible,background,1,random);
 for(auto &resident:residents)run(resident,1,random);
 portrait.player.select_page(mouth.page());run(portrait,0,random);
}
void shell_speech_scene::stop(random_state &random){
 if(finished())return;mouth.stop();
 portrait.player.select_page(mouth.page());run(portrait,0,random);
}
void shell_speech_scene::draw(framebuffer &target,uint8_t preference)const{target=visible;caption.draw(target,preference);}
}
