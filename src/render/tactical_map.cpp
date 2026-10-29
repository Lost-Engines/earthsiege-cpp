#include "tactical_map.h"
#include <algorithm>
#include "render/bitmap_raster.h"
#include "render/line_raster.h"
#include "resources/resource_store.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
int32_t initial_tactical_height(rectangle const &mission, rectangle const &screen) {
  return 2*std::max((((mission.right-mission.left)>>1)<<7)/(2*((screen.right-screen.left)>>1)),
                    (((mission.bottom-mission.top)>>1)<<7)/(2*((screen.bottom-screen.top)>>1)));
}
int32_t tactical_pan_step(int32_t const height) { return (((height-50000)>>3)*11875/118750)*8+5000; }
vec2<int32_t> tactical_project(vec3<int32_t> const &point, vec3<int32_t> const &camera, rectangle const &bounds) {
  int32_t const distance{camera.z-point.z};
  if(!distance) return {bounds.left,bounds.top};
  return {bounds.left+((bounds.right-bounds.left)>>1)+wrap_long(static_cast<int64_t>(point.x-camera.x)*128)/distance,
          bounds.top+((bounds.bottom-bounds.top)>>1)-wrap_long(static_cast<int64_t>(point.y-camera.y)*128)/distance};
}
tactical_map::tactical_map(resource_store const &resources, framebuffer &buffer, scenery_world const &source)
  : target{buffer}, world{source}, colours{source.colours()}, models{buffer,colours},
    icons{read_bitmaps(resources.read("simvol0","dba/ICONS.DBA"))} {colours.shade_ramps.at(14)={56};}
std::vector<tactical_hit> tactical_map::draw(rectangle const &bounds,tactical_map_state const &state) {
  screen_mask mask;
  for(int y{std::max(0,bounds.top)};y<=std::min(static_cast<int>(screen_height)-1,bounds.bottom);++y)
    mask.rows[static_cast<size_t>(y)].push_back({bounds.left,bounds.right});
  scoped_framebuffer_clip clip{target,&mask};
  target.fill(bounds,54);std::vector<tactical_hit> hits;
  if(state.failed) return hits;
  auto const origin{tactical_project({0,0,0},state.camera,bounds)};
  auto const far{tactical_project({800000,800000,0},state.camera,bounds)};
  int const span{far.x-origin.x};
  if(span>0) {
    for(int i{};origin.x+((i*span)>>4)<bounds.right;++i)
      target.fill({origin.x+((i*span)>>4),bounds.top,origin.x+((i*span)>>4),bounds.bottom},55);
    for(int i{-1},x{origin.x-(span>>4)};bounds.left<x;x=origin.x+((--i*span)>>4))
      target.fill({x,bounds.top,x,bounds.bottom},55);
    for(int i{};origin.y+((i*span)>>4)<bounds.bottom;++i)
      target.fill({bounds.left,origin.y+((i*span)>>4),bounds.right,origin.y+((i*span)>>4)},55);
    for(int i{-1},y{origin.y-(span>>4)};bounds.top<y;y=origin.y+((--i*span)>>4))
      target.fill({bounds.left,y,bounds.right,y},55);
  }
  auto const outline{[&](rectangle const &box,uint8_t colour){
    target.fill({box.left,box.top,box.right,box.top},colour);target.fill({box.left,box.bottom,box.right,box.bottom},colour);
    target.fill({box.left,box.top,box.left,box.bottom},colour);target.fill({box.right,box.top,box.right,box.bottom},colour);
  }};
  auto const lower{tactical_project({state.mission_bounds.left,state.mission_bounds.top,0},state.camera,bounds)};
  auto const upper{tactical_project({state.mission_bounds.right,state.mission_bounds.bottom,0},state.camera,bounds)};
  outline({lower.x,upper.y,upper.x,lower.y},53);
  auto camera{rotation_transform({49152,0,0})};camera.translation=state.camera;
  model_view view;view.camera=inverse_transform(camera);view.bounds=bounds;
  view.projection.perspective_shift=7;view.projection.near_distance=128;
  view.projection.centre={bounds.left+((bounds.right-bounds.left)>>1),bounds.top+((bounds.bottom-bounds.top)>>1)};
  std::array const lights{default_world_light()};
  {
  scoped_framebuffer_clip scenery_clip{target,nullptr};
  for(auto const &object:world.objects()) {
    auto instance{object.model};if(!instance.cell_frames().empty())instance.cell_frames().front()=1;
    models.draw(world.terrain().roots.at(object.shape),instance,object.placement,view,world.textures(),lights);
  }
  }
  std::optional<rectangle> selection;
  for(size_t i{};i<state.contacts.size();++i) {
    auto const &contact{state.contacts[i]};auto position{contact.position};
    if(contact.kind>=0) {
      if(contact.side && (contact.hidden || contact.dead))continue;
      if(!contact.detected && (contact.side || !contact.group_active)) {
        if((state.ticks&32) || contact.last_position_unavailable)continue;
        position={contact.last_position.x,contact.last_position.y,0};
      }
    }
    auto const centre{tactical_project(position,state.camera,bounds)};
    int const half{contact.size>>1};
    rectangle const hit{centre.x-half,centre.y-half,centre.x-half+contact.size,centre.y-half+contact.size};
    hits.push_back({hit,i});
    if(contact.destination) {
      auto const point{tactical_project(*contact.destination,state.camera,bounds)};
      draw_model_line(target,vec2<int32_t>{centre},vec2<int32_t>{point},bounds,contact.destination_colour);
    }
    int x{hit.left},y{hit.top};size_t frame{static_cast<size_t>(contact.frame)};
    if(contact.kind==0 || contact.kind==2) {
      size_t octant{};
      if(contact.heading>4096 && contact.heading<61440)octant=7-(contact.heading-4097)/8192;
      int const offset{std::array{0,0,0,0,0,-2,-4,-2}[octant]};x+=offset;y+=offset;
      if(!contact.dead)frame+=octant+1;
    }
    int drawn_half{half};
    if(!(contact.blink && state.blink_phase)) {
      auto const &icon{icons.at(frame)};
      if(contact.kind==1) {
        auto const distance{fast_distance({position.x-state.camera.x,position.y-state.camera.y,position.z-state.camera.z})};
        int const size{distance?3200000/distance:static_cast<int>(icon.width)};
        if(size<static_cast<int>(icon.height)) {
          draw_scaled_bitmap(target,icon,{x+half-(size>>1),y+half-(size>>1)},{size,size},bounds);drawn_half=size>>1;
        } else target.image(icon,x,y,true);
      } else target.image(icon,x,y,true);
    }
    if(contact.selected) selection=rectangle{hit.left-drawn_half,hit.top-drawn_half,hit.right+drawn_half-1,hit.bottom+drawn_half-1};
  }
  if(selection)outline(*selection,60);
  return hits;
}
} // namespace earthsiege
