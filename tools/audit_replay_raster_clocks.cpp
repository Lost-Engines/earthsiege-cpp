// Repaint independently clocked layers with the original external timer inputs.
// Expected clock/script state is only asserted, never assigned to the player.
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "shell/mission_view.h"
#include "resources/name_table.h"

using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=5)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESRCLK01")f.fail("clock audit format");
 auto demo{f.integer<uint32_t>()},initial{f.integer<uint32_t>()},count{f.integer<uint32_t>()};
 mission_view initial_view{resources,demo};
 auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
 auto const name{names.at(static_cast<size_t>(initial_view.simulation().player_chassis()))};
 auto const cockpit{read_cockpit(resources,name)};
 clock_instrument clock{resources,cockpit.layout,initial};
 presentation_player lamps{resources,"simvol0","ttm/"+name+".TTM"};
 std::filesystem::path source{argv[3]},output{argv[4]};std::filesystem::create_directories(output);
 size_t clock_reads{},lamp_reads{},states{},images{};
 for(unsigned i{};i<count;++i){
  auto kind{f.integer<uint8_t>()};
  if(kind==0)clock.refresh();
  else if(kind==1){
   auto now{f.integer<uint32_t>()},deadline{f.integer<uint32_t>()};
   if(clock.next_update()!=deadline)f.fail("clock deadline at event "+std::to_string(i));
   clock.update(now);++clock_reads;
  }else if(kind==2){
   auto elapsed{f.integer<int16_t>()};auto page{f.integer<uint8_t>()};
   auto mode{f.integer<int32_t>()};auto latch{f.integer<uint8_t>()};
   auto remaining{f.integer<int16_t>()},period{f.integer<int16_t>()},jump{f.integer<int16_t>()};
   auto const &state{lamps.state()};
   if(state.page!=page||state.mode!=mode||state.wait_latched!=bool(latch)||
      state.remaining!=remaining||state.period!=period||state.jump!=jump)
    f.fail("lamp state at event "+std::to_string(i));
   lamps.advance(elapsed);++lamp_reads;
  }else if(kind==3){
   auto frame_number{f.integer<uint32_t>()};auto text{f.cstring()};
   bool colon{f.integer<uint8_t>()!=0},inside{f.integer<uint8_t>()!=0};
   auto x{f.integer<int32_t>()},y{f.integer<int32_t>()};
   if(clock.text()!=text||clock.colon_visible()!=colon)f.fail("clock text at frame "+std::to_string(frame_number));
   ++states;
   std::ostringstream stem;stem<<"frame-"<<std::setw(5)<<std::setfill('0')<<frame_number;
   if(!std::filesystem::exists(source/(stem.str()+".bin")))continue;
   auto raster{read_file(source/(stem.str()+".bin"))};auto colours{read_file(source/(stem.str()+".pal"))};
   if(raster.size()!=64000||colours.size()!=768)f.fail("native diagnostic image dimensions");
   framebuffer frame;palette pal;
   for(size_t p{};p<256;++p){for(size_t c{};c<3;++c)pal.dac_colours[p][c]=colours[p*3+c];pal.colours[p]=display_colour(pal.dac_colours[p]);}
   for(int row{};row<200;++row)for(int col{};col<320;++col)frame.pixel(col,row,raster[static_cast<size_t>(row*320+col)]);
   if(inside){lamps.draw(frame,-x,-y);clock.draw(frame,-x,-y);}
   frame.save(output/(stem.str()+".ppm"),pal);++images;
  }else f.fail("unknown clock audit event");
 }
 f.require_end();
 std::cout<<clock_reads<<" original clock reads, "<<lamp_reads<<" original HMI dispatches, "<<states<<" clock states agree; "<<images<<" timed-layer images\n";
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
