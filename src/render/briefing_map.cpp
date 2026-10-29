#include "briefing_map.h"
#include "briefing_cosine_table.h"
#include "render/bitmap_raster.h"
#include "render/line_raster.h"
#include "render/polygon_raster.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"
#include <algorithm>
#include <limits>
namespace earthsiege {
namespace {
// GO's older 3Space angle approximation: truncate the ratio and use the
// signed-byte correction table at 0x8708c. DBSIM uses a later rounded table.
uint16_t map_bearing(vec3<int32_t> const &a,vec3<int32_t> const &b){
 static std::array<int8_t,513> constexpr correction{
  0,0,0,0,1,1,1,1,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,
  6,6,7,7,7,7,8,8,8,8,9,9,9,10,10,10,10,11,11,11,11,12,12,12,
  12,13,13,13,13,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,
  19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,22,23,23,23,23,24,24,24,
  24,25,25,25,25,25,26,26,26,26,27,27,27,27,27,28,28,28,28,29,29,29,29,29,
  30,30,30,30,30,31,31,31,31,31,32,32,32,32,32,33,33,33,33,33,34,34,34,34,
  34,34,35,35,35,35,35,35,36,36,36,36,36,36,37,37,37,37,37,37,38,38,38,38,
  38,38,38,39,39,39,39,39,39,39,40,40,40,40,40,40,40,41,41,41,41,41,41,41,
  41,41,42,42,42,42,42,42,42,42,42,43,43,43,43,43,43,43,43,43,43,44,44,44,
  44,44,44,44,44,44,44,44,44,44,44,45,45,45,45,45,45,45,45,45,45,45,45,45,
  45,45,45,45,45,45,45,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
  46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,
  46,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,44,44,44,
  44,44,44,44,44,44,44,44,44,44,43,43,43,43,43,43,43,43,43,43,43,42,42,42,
  42,42,42,42,42,42,41,41,41,41,41,41,41,41,40,40,40,40,40,40,40,39,39,39,
  39,39,39,39,38,38,38,38,38,38,37,37,37,37,37,37,36,36,36,36,36,36,35,35,
  35,35,35,34,34,34,34,34,33,33,33,33,33,32,32,32,32,32,31,31,31,31,31,30,
  30,30,30,29,29,29,29,29,28,28,28,28,27,27,27,27,26,26,26,26,25,25,25,25,
  24,24,24,24,23,23,23,23,22,22,22,21,21,21,21,20,20,20,20,19,19,19,18,18,
  18,18,17,17,17,16,16,16,15,15,15,15,14,14,14,13,13,13,12,12,12,11,11,11,
  10,10,10,10,9,9,9,8,8,8,7,7,7,6,6,5,5,5,4,4,4,3,3,3,
  2,2,2,1,1,1,0,0,0,
 };
 int const x{b.x-a.x},y{b.y-a.y},ax{std::abs(x)},ay{std::abs(y)};
 int angle{};
 if(ay){
  auto const index{static_cast<size_t>(static_cast<int64_t>(std::min(ax,ay))*512/std::max(ax,ay))};
  angle=static_cast<int>(index)+correction[index];
  if(ax<ay)angle=1024-angle;
 }
 if(x<0)angle=2048-angle;
 if(y<0)angle=4096-angle;
 return static_cast<uint16_t>(angle*16-16384);
}
int32_t map_cosine(uint16_t angle){
 int const index{std::abs(static_cast<int>(wrap_word(angle))>>4)};
 return index<1024?briefing_cosine_table[static_cast<size_t>(index)]:-briefing_cosine_table[static_cast<size_t>(2048-index)];
}
vec3<int32_t> fit(std::vector<vec3<int32_t>> const &points,int margin) {
 if(points.empty())return {0,0,200000};
 int minx{INT32_MAX},miny{INT32_MAX},maxx{INT32_MIN},maxy{INT32_MIN};
 for(auto const &p:points){minx=std::min(minx,p.x);miny=std::min(miny,p.y);maxx=std::max(maxx,p.x);maxy=std::max(maxy,p.y);}
 minx-=margin;miny-=margin;maxx+=margin;maxy+=margin;
 int const x{(maxx-minx)>>1},y{(maxy-miny)>>1};
 return {minx+x,miny+y,2*std::max((x<<7)/160,(y<<7)/100)};
}
vec2<int32_t> project(vec3<int32_t> const &p,vec3<int32_t> const &eye,rectangle const &r) {
 auto result{tactical_project(p,eye,r)};
 // The GO context stores -(width/2) with the negation before the shift.
 result.x+=(r.right-r.left)&1;result.y+=(r.bottom-r.top)&1;return result;
}
// Preserve the initially clipped line: it still consumes a reveal-clock tick.
std::vector<rectangle> grid_lines(vec3<int32_t> const &eye,rectangle bounds){
 auto const origin{project({0,0,0},eye,bounds)},distant{project({200000,200000,0},eye,bounds)};
 int const span{std::abs(distant.x-origin.x)};std::vector<rectangle> lines;
 if(span){
  for(int x{origin.x-((origin.x-bounds.left)/span+1)*span};x<bounds.right+((bounds.right-bounds.left)&1);x+=span)lines.push_back({x,bounds.top,x,bounds.bottom});
  for(int y{origin.y-((origin.y-bounds.top)/span+1)*span};y<bounds.bottom+((bounds.bottom-bounds.top)&1);y+=span)lines.push_back({bounds.left,y,bounds.right,y});
 }
 return lines;
}
}
briefing_map_geometry make_briefing_map(resource_store const &resources,mission_script const &script,int count){
 briefing_map_geometry result;result.points=script.points;result.bases=script.bases;
 result.player_count=count;result.hide_enemy=script.header.objective_kind>=0&&script.header.objective_kind<=2;
 auto const &group{script.groups.at(0)};result.members=group.members;
 if(group.route>=0)for(auto index:script.routes.at(static_cast<size_t>(group.route)).points)
  if(index>=0)result.route.push_back(script.points.at(static_cast<size_t>(index)));
 auto const anchor{group.position>=0?script.points.at(static_cast<size_t>(group.position)):result.route.at(0)};
 auto const heading{group.heading>=0?static_cast<uint16_t>(script.headings.at(static_cast<size_t>(group.heading))):
                    result.route.size()>1?map_bearing(result.route[0],result.route[1]):uint16_t{}};
 auto const cosine{map_cosine(heading)},sine{map_cosine(static_cast<uint16_t>(heading-16384))};
 auto raw{resources.read("simvol0","dat/MFORMS.DAT")};binary_reader forms{raw};
 std::array<vec3<int32_t>,7> offsets;
 if(group.formation>=0){
  forms.take(2+static_cast<size_t>(group.formation)*28);
  for(auto &p:offsets)p={forms.integer<int16_t>(),forms.integer<int16_t>(),0};
 }
 result.position_defined.fill(true);
 std::vector<vec3<int32_t>> squad,full;
 for(size_t i{};i<8;++i){
  if(group.members[i]>=0&&group.members[i]<count){
   // GO 0xcab1 leaves the local offsets uninitialized for formation -1.
   // Keep that uncertainty explicit instead of treating stack bytes as a map.
   if(i&&group.formation<0){result.position_defined[i]=false;continue;}
   result.players[i]=anchor;
   if(i){auto const &p{offsets[i-1]};result.players[i]+=vec3<int32_t>{wrap_word((p.x*cosine-p.y*sine+8192)>>14),wrap_word((p.x*sine+p.y*cosine+8192)>>14),0};}
  }
  if(group.members[i]!=-1)squad.push_back(result.players[i]);
 }
 full=squad;
 for(auto const &base:result.bases)if(base.position>=0)full.push_back(script.points.at(static_cast<size_t>(base.position)));
 full.insert(full.end(),result.route.begin(),result.route.end());
 result.full_view=fit(full,1000);result.squad_view=fit(squad,20000);
 result.minimum={INT32_MAX,INT32_MAX,INT32_MAX};result.maximum={INT32_MIN,INT32_MIN,INT32_MIN};
 for(auto const &p:script.points)for(unsigned axis{};axis<3;++axis){result.minimum[axis]=std::min(result.minimum[axis],p[axis]);result.maximum[axis]=std::max(result.maximum[axis],p[axis]);}
 for(unsigned axis{};axis<3;++axis){result.minimum[axis]-=50000;result.maximum[axis]+=50000;}
 for(auto const &g:script.groups)if(g.kind==actor_kind::base)
  for(auto index:g.members)if(index>=0)result.bases.at(static_cast<size_t>(index)).counter_operation=g.mode;
 return result;
}
briefing_map::briefing_map(resource_store const &resources,mission_script const &script,int count,std::vector<std::string> text)
 :colours{read_palette(resources.read("shell0","res32/REGN6000.DPL"))},
 textures{read_bitmaps(resources.read("shell0","dba/TEX1.DBA"))},icons{read_bitmaps(resources.read("shell0","dba/ICONS.DBA"))},
 text_face{read_font(resources.read("shell0","dfn/MAP.DFN"))},objectives{std::move(text)},
 data_{make_briefing_map(resources,script,count)}{
 if(script.header.zone>=0){
  terrain=read_model(resources.read("shell0","dgs/"+std::string{std::array{"TEMPER","ICE","DESERT","URBAN"}.at(static_cast<size_t>(script.header.terrain))}+".DGS"));
  auto const path{"dat/ZONE"+std::to_string(script.header.zone)+".DAT"};
  auto const volume{resources.archives().at("zones").find(path)?"zones":"simvol0"};
  auto zone{read_zone(resources.read(volume,path))};
  for(auto const &object:zone.placements){
   auto const &shape{std::get<shape_part>(terrain.roots.at(object.shape).content)};
   auto placement{rotation_transform(object.angles)};
   // Every authored ZONE placement is yaw-only. GO has an earlier cosine
   // table than DBSIM: its one-unit differences can move a projected edge.
   if(!object.angles.x&&!object.angles.y&&object.angles.z){
    auto c{static_cast<int16_t>(map_cosine(object.angles.z))},s{static_cast<int16_t>(map_cosine(static_cast<uint16_t>(object.angles.z-16384)))};
    placement.basis=mat3<int16_t>{c,s,int16_t{},static_cast<int16_t>(-s),c,int16_t{},int16_t{},int16_t{},model_matrix_one};
   }
   placement.translation=object.position;
   scenery.push_back({.shape{object.shape},.placement{placement},.model{model_instance{shape}}});
  }
 }
 auto const names{read_interface_strings(resources.read("shell0","str/MAPLABEL.STR"))};
 for(size_t i{};i<8;++i)if(data_.members[i]!=-1&&data_.position_defined[i])labels.push_back({vec3<int32_t>{data_.players[i]},names.at(0).at(i?1:0).text,7});
 for(auto const &b:data_.bases)if(b.counter_operation!=-1&&b.position>=0)
  labels.push_back({vec3<int32_t>{data_.points.at(static_cast<size_t>(b.position))},names.at(1).at(static_cast<size_t>(b.type)).text,b.type==5||b.type==6||b.type==8?7:13});
 colours.shade_ramps.at(14)={56};state_.camera=data_.full_view;
 state_.players=count;state_.route_points=static_cast<int>(data_.route.size());state_.waypoints=std::max(0,state_.route_points-1);display_state_=state_;
}

void briefing_map::start_tour(bool opening){
 state_={};state_.stage=0;state_.ready=false;state_.camera=data_.full_view;
 display_state_=state_;remaining_pages=2;route_index=1;text_latch=false;last_objective.clear();
 selected_label=-1;pan_step=20000;
 revealing=opening;reveal_clock_started=false;reveal_elapsed=0;
 reveal_duration=20+static_cast<uint32_t>(grid_lines(data_.full_view,{15,12,283,143}).size());
}
void briefing_map::skip_tour(){if(state_.stage<18){state_.stage=17;revealing=false;}}
void briefing_map::update(uint32_t now){
 auto &s{state_};int const texts{static_cast<int>(objectives.size())},routes{static_cast<int>(data_.route.size())};
 if(revealing){
  if(!reveal_clock_started){reveal_start=now;reveal_clock_started=true;}
  reveal_elapsed=now-reveal_start;
  if(reveal_elapsed<reveal_duration)return;
  revealing=false;s.deadline=reveal_start+reveal_duration+60;
 }
 auto const paint{[&](){
  display_state_=s;
  if((s.stage==7||s.stage==13)&&s.objective>=0&&s.objective<texts&&!objectives[static_cast<size_t>(s.objective)].empty())
   last_objective=objectives[static_cast<size_t>(s.objective)];
 }};
 auto const move{[&](vec3<int32_t> const &to,int duration){
  from=s.camera;destination=to;segment_ticks=duration;
  for(unsigned axis{};axis<3;++axis)increment[axis]=(to[axis]-from[axis])/duration;
 }};
 auto const interpolate{[&](int duration){
  if(now<s.deadline){int const elapsed{duration-static_cast<int>(s.deadline-now)};s.camera=from+increment*elapsed;return false;}
  s.camera=destination;return true;
 }};
 // Fallthroughs are significant: the original can draw and change state in
 // the same call, while other transitions wait for the next shell iteration.
 switch(s.stage){
 case 0:
  if(!s.deadline){s.camera=data_.full_view;paint();s.deadline=now+60;break;}
  if(now<s.deadline){paint();break;}
  [[fallthrough]];
 case 1:
  s.deadline+=120;move(data_.squad_view,120);s.stage=2;
  [[fallthrough]];
 case 2:
  if(interpolate(120)){paint();s.stage=4;}else paint();break;
 case 3:
  if(now<s.deadline){paint();break;}
  ++s.players;
  [[fallthrough]];
 case 4:
  paint();
  if(s.players<data_.player_count){s.deadline=now+30;s.stage=3;}
  else {route_index=1;s.objective=0;s.resume=5;s.deadline=now+30;s.stage=15;}
  break;
 case 5:
  if(s.objective<texts){
   if(!routes||route_index<routes){s.deadline=now+30;s.resume=6;s.stage=13;paint();break;}
   --s.objective;text_latch=true;goto remaining_text;
  }
  [[fallthrough]];
 case 6:
  ++s.route_points;++s.waypoints;++route_index;
  if(!routes||route_index>routes){s.stage=17;break;}
  {auto point{data_.route.at(static_cast<size_t>(route_index-1))};point.z=s.camera.z;
   int const duration{(fast_distance(s.camera-point)/50000+1)*30};
   move(point,duration);s.deadline=now+static_cast<uint32_t>(duration);s.stage=7;}
  [[fallthrough]];
 case 7:
  if(!interpolate(segment_ticks)){paint();break;}
  paint();s.stage=8;
  [[fallthrough]];
 case 8:
  if(s.objective<texts){
   int const delay{static_cast<int>(objectives.at(static_cast<size_t>(s.objective)).size())*3-segment_ticks};
   if(delay>0){s.deadline=now+static_cast<uint32_t>(delay);s.resume=9;s.stage=13;break;}
  }
  [[fallthrough]];
 case 9:
  ++s.objective;
  if(s.objective<texts){s.deadline=now+30;s.resume=10;s.stage=14;}
  else if(s.objective==texts&&routes&&route_index<routes){--s.objective;s.stage=5;}
  else{s.deadline=now+30;s.resume=10;s.stage=15;}
  break;
 case 10:
  if(s.objective<texts){s.resume=11;s.stage=12;break;}
  [[fallthrough]];
 case 11:
  if(s.objective<texts){++s.objective;s.deadline=now+30;s.resume=5;s.stage=14;}
  else s.stage=5;
  break;
 case 12:
  if(s.objective<texts){s.deadline=now+static_cast<uint32_t>(objectives.at(static_cast<size_t>(s.objective)).size()*3);s.stage=13;}
  [[fallthrough]];
 case 13:case 14:case 15:
  if(now<s.deadline)paint();else s.stage=s.resume;
  break;
 case 16:
 remaining_text:
  if(s.objective<texts){
   if(text_latch&&!objectives.at(static_cast<size_t>(s.objective)).empty()){
    s.deadline=now+30;s.resume=16;s.stage=14;text_latch=false;break;
   }
   ++s.objective;while(s.objective<texts&&objectives.at(static_cast<size_t>(s.objective)).empty())++s.objective;
   s.resume=16;s.stage=12;text_latch=true;
  }else s.stage=5;
  break;
 case 17:
  s.players=data_.player_count;
  if(routes){s.route_points=routes;s.waypoints=routes-1;}
  s.deadline=now+120;move(data_.full_view,120);s.stage=18;
  [[fallthrough]];
 case 18:
  if(interpolate(120))s.stage=19;
  paint();break;
 case 19:
  paint();if(!s.ready&&!--remaining_pages)s.ready=true;break;
 }
}

void briefing_map::navigate(uint16_t action){
 if(!state_.ready)return;
 auto &c{state_.camera};auto &p{state_.pan};
 switch(action){
 case 108:if(c.z-50000>=50000){c.z-=50000;pan_step=tactical_pan_step(c.z);}break;
 case 109:if(c.z+50000<=1000000){c.z+=50000;pan_step=tactical_pan_step(c.z);}if(c.z>data_.full_view.z){c.z=data_.full_view.z;pan_step=tactical_pan_step(c.z);}break;
 case 1503:if(c.y+pan_step<=5000000)p.y+=pan_step;break;
 case 1504:if(c.y-pan_step>=-5000000)p.y-=pan_step;break;
 case 1505:if(c.x-pan_step>=-5000000)p.x-=pan_step;break;
 case 1506:if(c.x+pan_step<=5000000)p.x+=pan_step;break;
 }
 display_state_=state_;
}
void briefing_map::restore_view(){
 // GO 0x9c64 restores the full fitted camera on a screen rebuild; pan remains.
 state_.camera=data_.full_view;display_state_=state_;
}
void briefing_map::hover(int x,int y,rectangle bounds){
 if(!state_.ready||!bounds.contains(x,y))return;
 auto eye{state_.camera};eye.x+=state_.pan.x;eye.y+=state_.pan.y;
 for(size_t i{};i<labels.size();++i){
  auto const &label{labels[i]};auto p{project(label.position,eye,bounds)};p.x-=label.size>>1;p.y-=label.size>>1;
  if(rectangle{p.x,p.y,p.x+label.size,p.y+label.size}.contains(x,y)){selected_label=static_cast<int>(i);return;}
 }
 // Original selection remains latched after the pointer leaves its hotspot.
}
void briefing_map::draw(framebuffer &target,rectangle bounds){
 screen_mask mask;for(int y{std::max(0,bounds.top)};y<=std::min(199,bounds.bottom);++y)mask.rows[static_cast<size_t>(y)].push_back({bounds.left,bounds.right});
 mask.regions.push_back({bounds.top,std::vector<scanline_span>(static_cast<size_t>(bounds.bottom-bounds.top+1),{bounds.left,bounds.right}),true});
 scoped_framebuffer_clip clip{target,&mask};
 auto eye{display_state_.camera};eye.x+=display_state_.pan.x;eye.y+=display_state_.pan.y;
 auto const screen{[&](vec3<int32_t> const &p){return project(p,eye,bounds);}};
 target.fill(bounds,54);
 if(revealing&&reveal_elapsed<10)return;
 auto transform{rotation_transform({49152,0,0})};transform.translation=eye;
 model_view view;view.camera=inverse_transform(transform);view.bounds=bounds;view.detail.projection_shift=7;view.projection.perspective_shift=7;view.projection.near_distance=128;
 view.projection.centre={bounds.left+((bounds.right-bounds.left+1)>>1),bounds.top+((bounds.bottom-bounds.top+1)>>1)};
 model_renderer models{target,colours};std::array const lights{default_world_light()};
 {scoped_framebuffer_clip scenery_clip{target,nullptr};
 for(auto const &object:scenery){
  auto instance{object.model};if(!instance.cell_frames().empty())instance.cell_frames().front()=1;
  models.draw(terrain.roots.at(object.shape),instance,object.placement,view,textures,lights);
 }}
 if(revealing&&reveal_elapsed<20)return;
 auto const grid{grid_lines(eye,bounds)};
 size_t const shown_lines{revealing?std::min(grid.size(),static_cast<size_t>(reveal_elapsed-20+1)):grid.size()};
 for(size_t i{};i<shown_lines;++i)target.fill(grid[i],55);
 if(revealing)return;
 auto const lower{screen({data_.minimum.x,data_.minimum.y,0})},upper{screen({data_.maximum.x,data_.maximum.y,0})};
 target.fill({lower.x,upper.y,upper.x,upper.y},53);target.fill({lower.x,lower.y,upper.x,lower.y},53);
 target.fill({lower.x,upper.y,lower.x,lower.y},53);target.fill({upper.x,upper.y,upper.x,lower.y},53);
 if(display_state_.route_points==static_cast<int>(data_.route.size()))
  for(size_t i{1};i<data_.route.size();++i)draw_model_line(target,screen(data_.route[i-1]),screen(data_.route[i]),bounds,56);
 else if(display_state_.route_points>0&&!data_.route.empty()){
  auto const head{screen({display_state_.camera.x,display_state_.camera.y,0})};
  draw_model_line(target,screen(data_.route.at(static_cast<size_t>(display_state_.route_points-1))),{head.x,head.y},bounds,56);
  // Original ellipse scan conversion excludes the seventh/bottom scanline.
  std::array constexpr radii{1,2,3,3,2,1};
  for(size_t y{};y<radii.size();++y)target.fill({head.x-radii[y],head.y+static_cast<int>(y)-3,head.x+radii[y],head.y+static_cast<int>(y)-3},56);
 }
 auto const icon{[&](vec3<int32_t> const &position,int index,int authored,int minimum,int size,int dot){
  auto const centre{screen(position)};auto const &bitmap{icons.at(static_cast<size_t>(index))};int const extent{(size<<7)/display_state_.camera.z};
  if(extent<minimum)target.pixel(centre.x,centre.y,static_cast<uint8_t>(dot));
  else if(extent<static_cast<int>(bitmap.height))draw_shell_scaled_bitmap(target,bitmap,{centre.x-(extent>>1),centre.y-(extent>>1)},{extent,extent},bounds);
  else target.image(bitmap,centre.x-(authored>>1),centre.y-(authored>>1),true);
 }};
 for(auto const &base:data_.bases)if(base.counter_operation>=0&&base.position>=0&&!(data_.hide_enemy&&base.counter_operation==1)){
  bool const enemy{base.counter_operation==1},small{base.type==5||base.type==6||base.type==8};
  icon(data_.points.at(static_cast<size_t>(base.position)),(enemy?72:74)+(small?1:0),small?7:13,small?1:0,25000,enemy?53:50);
 }
 for(int i{1};i<static_cast<int>(data_.route.size())&&i<=9&&i<=display_state_.waypoints;++i){auto p{screen(data_.route[static_cast<size_t>(i)])};target.image(icons.at(static_cast<size_t>(i-1)),p.x-4,p.y-4,true);}
 for(size_t i{},shown{};i<8&&shown<static_cast<size_t>(display_state_.players);++i)if(data_.members[i]!=-1){if(data_.position_defined[i])icon(data_.players[i],data_.members[i]==0?18:27,7,1,100000,50);++shown;}
 if(display_state_.stage==19){int y{bounds.top+9};for(auto const &text:objectives)if(!text.empty()){target.text(text_face,text,bounds.left,y);y+=10;}}
 else if(display_state_.stage==7||display_state_.stage==13||display_state_.stage==14){
  int const index{display_state_.objective};std::string text;
  if(index>=0&&index<static_cast<int>(objectives.size()))text=objectives[static_cast<size_t>(index)];
  if(display_state_.stage==14){if(text.empty())text=last_objective;else text.clear();}
  else if(index<static_cast<int>(objectives.size())&&text.empty())text=last_objective;
  int const x{bounds.left+((bounds.right-bounds.left)>>1)-((text_face.text_width(text)-1)>>1)};
  target.text(text_face,text,x,bounds.top+9);
 }
 if(display_state_.stage==19&&selected_label>=0){
  auto const &label{labels.at(static_cast<size_t>(selected_label))};auto const p{screen(label.position)};
  int const local_left{p.x-(label.size>>1)-bounds.left},width{6+text_face.text_width(label.text)};
  bool const left{bounds.left+((bounds.right-bounds.left)>>1)<local_left};
  int const x{left?local_left-6-width+bounds.left:local_left+label.size+5+bounds.left};
  rectangle const box{x,p.y-5,x+width,p.y+5};
  target.fill(box,63);
  target.text(text_face,label.text,x+(width>>1)-((text_face.text_width(label.text)-1)>>1),box.top+9);
  target.fill({box.left,box.top,box.right,box.top},59);target.fill({box.left,box.bottom,box.right,box.bottom},59);
  target.fill({box.left,box.top,box.left,box.bottom},59);target.fill({box.right,box.top,box.right,box.bottom},59);
  int const tip{left?box.right+6:box.left-6},base{left?box.right+1:box.left-1};
  std::array<vec2<int32_t>,3> const triangle{vec2<int32_t>{tip,p.y},vec2<int32_t>{base,p.y+(left?2:-2)},vec2<int32_t>{base,p.y+(left?-2:2)}};
  draw_solid_polygon(target,triangle,bounds,59);
 }
}
}
