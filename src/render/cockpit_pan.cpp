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

void cockpit_surface::copy_page(int source_x,int target_x,rectangle bounds){
  if(source_x%4||target_x%4)throw std::invalid_argument{"unaligned VGA page origin"};
  auto group=[&](int x,int y,unsigned mask){
    std::array<uint8_t,4> latch{};
    auto const source{y*640+source_x+x},target{y*640+target_x+x};
    if(source<0||target<0||source+3>=static_cast<int>(pixels_.size())||target+3>=static_cast<int>(pixels_.size()))
      throw std::out_of_range{"VGA page copy outside retained surface"};
    for(unsigned plane{};plane<4;++plane)latch[plane]=pixels_[source+plane];
    for(unsigned plane{};plane<4;++plane)if(mask&(1U<<plane))pixels_[target+plane]=latch[plane];
  };
  // Original 51cxx: partial left column, then all complete VGA bytes,
  // then partial right column. Each read latches four pixels atomically.
  if(bounds.left&3)for(int y{bounds.top};y<=bounds.bottom;++y)
    group(bounds.left&~3,y,(15U<<(bounds.left&3))&15U);
  int const first{(bounds.left+3)&~3},end{(bounds.right+1)&~3};
  for(int y{bounds.top};y<=bounds.bottom;++y)for(int x{first};x<end;x+=4)group(x,y,15);
  if((bounds.right+1)&3)for(int y{bounds.top};y<=bounds.bottom;++y)
    group(bounds.right&~3,y,(1U<<((bounds.right+1)&3))-1);
}
void cockpit_surface::restore_from_external(int source_x,int target_x){
  // 22b4d uses inclusive right=320, so the final masked pass crosses a
  // page boundary and overwrites the source page's first column.
  copy_page(source_x,target_x,{0,0,320,199});
  retained_edge.at(static_cast<size_t>(source_x/320))=true;
}
void cockpit_surface::restore_artwork_edge(framebuffer &frame,int x,int y)const{
  if(!retained_edge.at(static_cast<size_t>(x/320)))return;
  for(int row{};row<200;++row)if(y+row>=0&&y+row<400)
    frame.pixel(0,row,pixels_[static_cast<size_t>((y+row)*640+x)]);
}
} // namespace earthsiege
