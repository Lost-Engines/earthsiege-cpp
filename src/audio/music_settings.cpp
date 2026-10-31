#include "music_settings.h"
#include <array>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
namespace earthsiege {
namespace {
constexpr std::array names{"opl2","opl3","mt32","mt32-mpu","sc55","scc1a","soundfont"};
constexpr std::array labels{"Sound Blaster / AdLib (OPL2)","Sound Blaster Pro / 16 (OPL3)","MT-32 / CM-32L (original routing)","MT-32 / CM-32L via MPU-401","Sound Canvas SC-55","Sound Canvas SCC-1A","General MIDI SoundFont"};
}
std::string_view music_device_name(music_device d){return names.at(static_cast<size_t>(d));}
std::string_view music_device_label(music_device d){return labels.at(static_cast<size_t>(d));}
music_device parse_music_device(std::string_view name){
  if(name=="roland-mt32"||name=="roland-lapc")return music_device::mt32;
  if(name=="roland-sc55")return music_device::sc55;
  if(name=="roland-scc1a")return music_device::scc1a;
  if(name=="midi")return music_device::soundfont;
  for(size_t i{};i<names.size();++i)if(name==names[i])return static_cast<music_device>(i);
  throw std::runtime_error{"unknown music device: "+std::string{name}};
}
uint32_t music_driver_id(music_device d){
  switch(d){case music_device::opl2:return 0xa002;case music_device::opl3:return 0xa009;case music_device::mt32:return 0xa004;default:return 0xa001;}
}
std::filesystem::path music_asset_directory(std::filesystem::path const &program){
#ifdef _WIN32
  return program.empty()?std::filesystem::current_path():program;
#else
  (void)program;
  if(auto const *xdg=std::getenv("XDG_DATA_HOME");xdg&&std::filesystem::path{xdg}.is_absolute())return std::filesystem::path{xdg}/"earthsiege";
  if(auto const *home=std::getenv("HOME");home&&*home)return std::filesystem::path{home}/".local/share/earthsiege";
  return "assets";
#endif
}
std::vector<std::filesystem::path> music_asset_directories(music_settings const &s,std::filesystem::path const &data){
  if(!s.rom_directory.empty())return {s.rom_directory};
  std::vector<std::filesystem::path> result;
  for(auto const &base:{data,s.program_directory,music_asset_directory(s.program_directory)}){
    if(base.empty())continue;
    for(auto const &candidate:{base,base/"roms"})if(std::ranges::find(result,candidate)==result.end())result.push_back(candidate);
  }
  return result;
}
music_settings read_music_settings(std::filesystem::path const &file){
  music_settings result;if(!std::filesystem::exists(file))return result;
  std::ifstream in{file};std::string key,value;
  if(!in)throw std::runtime_error{"cannot read music settings: "+file.string()};
  while(in>>key){
    if(!(in>>std::quoted(value)))throw std::runtime_error{"invalid music setting: "+key};
    if(key=="device")result.device=parse_music_device(value);
    else if(key=="rom_directory")result.rom_directory=value;
    else if(key=="soundfont")result.soundfont=value;
    else throw std::runtime_error{"unknown music setting: "+key};
  }
  if(!in.eof())throw std::runtime_error{"invalid music settings: "+file.string()};
  return result;
}
void write_music_settings(std::filesystem::path const &file,music_settings const &s){
  if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path());
  auto temporary{file};temporary+=".tmp";
  {std::ofstream out{temporary};out<<"device "<<music_device_name(s.device)<<"\nrom_directory "<<std::quoted(s.rom_directory.string())<<"\nsoundfont "<<std::quoted(s.soundfont.string())<<'\n';out.close();if(!out)throw std::runtime_error{"cannot save music settings: "+file.string()};}
  std::filesystem::rename(temporary,file);
}
}
