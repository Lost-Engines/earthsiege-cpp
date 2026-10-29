#pragma once
#include <functional>
#include "shell/speech_selection.h"
#include "resources/shell_speech.h"
#include "render/shell_caption.h"
#include "render/shell_animation_artwork.h"
namespace earthsiege {
/// GO's portrait plus selected resident programs and original captions.
class shell_speech_scene {
public:
 using resident_update=std::function<void(framebuffer &,framebuffer &,int16_t,random_state &)>;
private:
 struct animated {
  shell_animation player;
  shell_animation_artwork art;
  animated(resource_store const &r,std::string const &name):player{read_presentation(r.read("shell0","res32/"+name))},art{r}{}
 };
 resident_update external;
 std::vector<animated> residents;
 animated portrait;
 shell_mouth mouth;
 shell_caption caption;
 std::string sample_;
 framebuffer background,visible;
 uint16_t clock{};
 void run(animated &,int16_t elapsed,random_state &);
public:
 shell_speech_scene(resource_store const &,speech_selection const &,framebuffer const &base,random_state &,framebuffer const *retained=nullptr,resident_update external={});
 void advance(random_state &);
 void stop(random_state &);
 void draw(framebuffer &,uint8_t speech_preference)const;
 bool finished()const{return !mouth.active();}
 std::string const &sample()const{return sample_;}
 framebuffer const &image()const{return visible;}
 framebuffer const &retained()const{return background;}
 uint16_t mouth_page()const{return mouth.page();}
};
}
