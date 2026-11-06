#include "shell_cinematic.h"
namespace earthsiege {
shell_cinematic::shell_cinematic(resource_store const &r,std::string const &path,random_state &random)
 {
 if(normalise_path(path).ends_with(".dfl")){
  auto file{r.external(path)};if(!file)throw std::runtime_error{"film not found: "+path};
  film=std::make_unique<dfl_movie>(read_file(*file));film_present();
 }else{
  player=std::make_unique<shell_animation>(read_presentation(r.read("shell0",path)));
  art=std::make_unique<shell_animation_artwork>(r,path.substr(0,path.find_last_of('/')+1));execute(0,random);present();
 }
}
void shell_cinematic::film_present(){
 auto const pixels{film->pixels()};for(int y{};y<200;++y)for(int x{};x<320;++x)shown.pixel(x,y,pixels[static_cast<size_t>(y*320+x)]);colours_=film->colours();
}

void shell_cinematic::execute(int16_t elapsed,random_state &random){
 visible=background;player->advance(elapsed,random);art->apply(player->commands(),visible,background);
 for(auto const &frame:art->palette_frames())pending.push_back(frame);
 pending.push_back({visible,art->colours()});
}
void shell_cinematic::present(){
 if(pending.empty())return;
 shown=std::move(pending.front().image);colours_=std::move(pending.front().colours);pending.pop_front();
}
void shell_cinematic::advance(random_state &random){
 if(film){film_clock+=1000;if(film_clock>=67*60){film_clock-=67*60;film->advance();film_present();}return;}
 if(pending.empty())execute(1,random);
 present();
}
}
