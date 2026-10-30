#include "cockpit_pan.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
std::vector<vec2<int32_t>> cockpit_pan_path(int action,int view,
    std::array<cockpit_window,4> const &windows){
  std::vector<vec2<int32_t>> path;
  auto origin=[&](int index){return windows.at(static_cast<size_t>(index)).instrument_origin;};
  if(action==0&&view==0){
    int const distance{origin(1)[1]-origin(0)[1]};
    // Downward scrolling overshoots when the authored displacement is not
    // divisible by five; the last write corrects the display origin.
    for(int y{};y<distance;){y+=5;path.push_back({0,y});}
    path.push_back({0,distance});
  }else if(action==1&&view==1){
    int y{origin(1)[1]-origin(0)[1]};
    for(;y>=5;){y-=5;path.push_back({0,y});}
    path.push_back({0,0});
  }else if(action==4||action==5||action==6){
    int distance{},direction{};
    if(action==4){distance=origin(2)[0]-origin(0)[0];direction=1;}
    else if(action==5){distance=-(origin(0)[0]+origin(3)[0]);direction=-1;}
    else if(view==2){distance=origin(0)[0]+origin(2)[0];direction=-1;}
    else if(view==3){distance=-(origin(0)[0]+origin(3)[0]);direction=1;}
    else return path;
    for(int travelled{};travelled<distance;){
      travelled+=10;path.push_back({direction*travelled,0});
    }
    path.push_back({direction*distance,0});
  }
  return path;
}
void cockpit_surface::restore(std::span<uint8_t const> source){
  if(source.size()!=pixels_.size())throw std::invalid_argument{"cockpit surface size"};
  std::ranges::copy(source,pixels_.begin());
}
void cockpit_surface::store(framebuffer const &frame,int x,int y){
  for(int row{};row<200;++row)for(int column{};column<320;++column)
    if(x+column>=0&&x+column<640&&y+row>=0&&y+row<400)
      pixels_[static_cast<size_t>((y+row)*640+x+column)]=frame.pixels()[static_cast<size_t>(row*320+column)];
}
void cockpit_surface::sample(framebuffer &frame,int x,int y)const{
  if(x<0||y<0||x+320>640||y+200>400)throw std::out_of_range{"cockpit scanout outside surface"};
  scoped_framebuffer_clip clip{frame,nullptr};
  for(int row{};row<200;++row)for(int column{};column<320;++column)
    frame.pixel(column,row,pixels_[static_cast<size_t>((y+row)*640+x+column)]);
}
} // namespace earthsiege
