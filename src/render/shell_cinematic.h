#pragma once
#include "render/shell_animation_artwork.h"
#include <deque>
namespace earthsiege {
/// Nonblocking owner for the original blocking TTM sequences. Palette writes
/// retain their order; their host display cadence is separate from TTM waits.
class shell_cinematic {
 shell_animation player;
 shell_animation_artwork art;
 framebuffer visible,background,shown;
 palette colours_;
 std::deque<shell_palette_frame> pending;
 void execute(int16_t elapsed,random_state &);
 void present();
public:
 shell_cinematic(resource_store const &,std::string const &path,random_state &);
 void advance(random_state &);
 bool finished()const{return pending.empty()&&player.state().mode==2;}
 framebuffer const &image()const{return shown;}
 palette const &colours()const{return colours_;}
};
}
