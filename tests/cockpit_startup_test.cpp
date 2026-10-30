#include "shell/cockpit_startup.h"
#include "shell/mission_view.h"
#include "render/framebuffer.h"
#include "resources/resource_store.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <fstream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESSTART1")f.fail("startup fixture version");
 auto const height{f.integer<int32_t>()};rectangle bounds;
 bounds.left=f.integer<int32_t>();bounds.top=f.integer<int32_t>();bounds.right=f.integer<int32_t>();bounds.bottom=f.integer<int32_t>();
 camera_projection projection;projection.centre.x=f.integer<int32_t>();projection.centre.y=f.integer<int32_t>();
 auto palette_read=[&]{palette p;for(auto &c:p.dac_colours)for(auto &v:c)v=f.integer<uint8_t>();for(size_t i{};i<256;++i)p.colours[i]=display_colour(p.dac_colours[i]);return p;};
 auto const target{palette_read()},source{palette_read()};cockpit_startup start{target};
 if(start.colours().dac_colours!=source.dac_colours)f.fail("initial startup palette");
 random_state::state rng;for(auto &v:rng.words)v=f.integer<uint16_t>();rng.first=f.integer<uint8_t>();rng.second=f.integer<uint8_t>();random_state random{rng};
 resource_store resources{argv[2]};mission_view live{resources,1};
 live.begin_startup();if(!live.starting())f.fail("nearby-base startup missing");
 auto const player_position{live.simulation().player_actor().body().movement().position};
 auto const simulation_rng{live.simulation().random_snapshot()};
 auto lifts{f.integer<uint16_t>()};
 for(unsigned i{};i<lifts;++i){
  auto draw_clock{f.integer<uint32_t>()},palette_clock{f.integer<uint32_t>()},present_clock{f.integer<uint32_t>()};
  (void)draw_clock;auto expected_height{f.integer<int32_t>()},expected_bottom{f.integer<int32_t>()};
  if(!start.lifting()||height+start.height_offset()!=expected_height)f.fail("original lift height");
  auto cover{start.cover_y(height+start.height_offset(),projection)};
  if(std::clamp(cover,bounds.top,bounds.bottom)!=expected_bottom)f.fail("original lift clipping");
  start.prepare_frame(random);start.update_palette(palette_clock,cover);
  auto const expected{palette_read()};
  if(start.colours().dac_colours!=expected.dac_colours)f.fail("original lift palette at "+std::to_string(i));
  start.presented(present_clock);
  live.draw_startup(draw_clock);live.finish_startup_frame(palette_clock,present_clock);
  if(auto const *path{std::getenv("EARTHSIEGE_STARTUP_IMAGES")}){
    std::filesystem::create_directories(path);
    live.image().save(std::filesystem::path{path}/("lift-"+std::to_string(i)+".ppm"),live.colours());
    std::ofstream pal{std::filesystem::path{path}/("lift-"+std::to_string(i)+".pal"),std::ios::binary};
    for(auto const &c:live.colours().dac_colours)pal.write(reinterpret_cast<char const *>(c.data()),3);
  }
  for(unsigned first:{32U,64U})for(unsigned colour{first};colour<first+16;++colour)
   if(live.colours().dac_colours[colour]!=expected.dac_colours[colour])
    f.fail("integrated lift palette frame "+std::to_string(i)+" colour "+std::to_string(colour));

 }
 if(live.simulation().interface_random().snapshot().words!=rng.words ||
    live.simulation().interface_random().snapshot().first!=rng.first ||
    live.simulation().interface_random().snapshot().second!=rng.second)
   f.fail("startup environment RNG before landing");
 auto shakes{f.integer<uint16_t>()};
 for(unsigned i{};i<shakes;++i){
  auto draw_clock{f.integer<uint32_t>()},end_clock{f.integer<uint32_t>()};(void)draw_clock;
  auto displacement{f.integer<int16_t>()};
  start.prepare_frame(random);
  if(start.lifting()||start.displacement()!=displacement)f.fail("original landing shake");
  auto const &actual{random.snapshot()};
  for(auto v:actual.words)if(v!=f.integer<uint16_t>())f.fail("original startup visual random word");
  if(actual.first!=f.integer<uint8_t>()||actual.second!=f.integer<uint8_t>())f.fail("original startup visual random index");
  live.draw_startup(draw_clock);
  auto const &live_rng{live.simulation().interface_random().snapshot()};
  if(live_rng.words!=actual.words||live_rng.first!=actual.first||live_rng.second!=actual.second)
   f.fail("integrated landing visual RNG");
  live.finish_startup_frame(draw_clock,end_clock);
  start.drawn(end_clock);
  if(live.starting()!=(i+1!=shakes))f.fail("integrated launch completion");

  if(start.finished()!=(i+1==shakes))f.fail("original startup expiry");
 }
 if(live.simulation().player_actor().body().movement().position!=player_position ||
    live.simulation().random_snapshot().words!=simulation_rng.words)
  f.fail("startup advanced simulation");
 // All four unchanged DOSBox startup snapshots finish on the same page pair.
 if(live.presentation_state().draw_page!=2||live.presentation_state().other_page!=0)
  f.fail("startup cockpit page ownership");
 f.require_end();std::cout<<lifts<<" lift frames and "<<shakes<<" shake frames match original palette, camera, clipping and random state\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
