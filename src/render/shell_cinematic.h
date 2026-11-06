#pragma once
#include "render/shell_animation_artwork.h"
#include <deque>
#include <memory>
#include "resources/dfl_movie.h"
namespace earthsiege {
/// Nonblocking owner for the original blocking TTM sequences. Palette writes
/// retain their order; their host display cadence is separate from TTM waits.
class shell_cinematic {
 std::unique_ptr<shell_animation> player;
 std::unique_ptr<shell_animation_artwork> art;
 std::unique_ptr<dfl_movie> film;
 unsigned film_clock{};
 void film_present();
 framebuffer visible,background,shown;
 palette colours_;
 std::deque<shell_palette_frame> pending;
 void execute(int16_t elapsed,random_state &);
 void present();
public:
 shell_cinematic(resource_store const &,std::string const &path,random_state &);
 void advance(random_state &);
 bool finished()const{return film?film->finished():pending.empty()&&player->state().mode==2;}
 bool is_film()const{return film!=nullptr;}
 std::vector<std::string> take_samples(){return film?film->take_samples():std::vector<std::string>{};}
 framebuffer const &image()const{return shown;}
 palette const &colours()const{return colours_;}
};
}
