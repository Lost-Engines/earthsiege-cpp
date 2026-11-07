#include "shell/mission_alert.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESOBJ001")f.fail("fixture format");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto size{f.integer<uint16_t>()};auto pressed{f.integer<uint8_t>()};auto strings{read_interface_strings(f.take(size))};auto expected{f.take(64000)};
  std::vector<std::string> lines;for(auto const &line:strings.at(0))lines.push_back(line.text);
  mission_alert dialog{resources,lines};framebuffer frame;frame.clear(37);
  if(pressed)dialog.mouse_down(159,143);
  dialog.draw(frame);size_t errors{};
  for(size_t p{};p<expected.size();++p)if(frame.pixels()[p]!=expected[p])++errors;
  if(errors){std::ofstream out{"/tmp/es-objectives-actual.raw",std::ios::binary};out.write(reinterpret_cast<char const *>(frame.pixels().data()),64000);f.fail("F11 raster "+std::to_string(i)+": "+std::to_string(errors)+" mismatched pixels");}
  if(dialog.exits_mission(0)||dialog.key(1)!=0)f.fail("objectives dismiss without exiting mission");
 }
 f.require_end();std::cout<<count<<" original F11 objectives rasters match\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
