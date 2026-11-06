#include "save_store.h"
#include "resources/binary_writer.h"
#include <fstream>
#include <stdexcept>
namespace earthsiege {
std::filesystem::path save_store::path(std::string_view name)const{
  auto normal{normalise_path(name)};std::filesystem::path relative{normal};
  if(relative.empty()||relative.is_absolute()||relative.has_root_name())throw std::runtime_error{"invalid save filename"};
  for(auto const &component:relative)if(component=="..")throw std::runtime_error{"save filename leaves save directory"};
  // Accept imported DOS saves with upper-case names on case-sensitive hosts.
  auto resolved{directory};
  for(auto const &component:relative){auto next{resolved/component};
    if(!std::filesystem::exists(next)&&std::filesystem::is_directory(resolved)){
      for(auto const &file:std::filesystem::directory_iterator{resolved})if(normalise_path(file.path().filename().string())==component.string()){next=file.path();break;}
    }
    resolved=std::move(next);
  }
  return resolved;
}
std::optional<std::vector<uint8_t>> save_store::read(std::string_view name)const{
  auto file{path(name)};if(!std::filesystem::exists(file))return {};return read_file(file);
}
void save_store::write(std::string_view name,bytes data)const{
  auto target{path(name)};std::filesystem::create_directories(target.parent_path());auto temporary{target};temporary+=".tmp";
  {std::ofstream file{temporary,std::ios::binary|std::ios::trunc};file.write(reinterpret_cast<char const *>(data.data()),static_cast<std::streamsize>(data.size()));file.close();if(!file)throw std::runtime_error{"cannot save "+target.string()};}
  std::filesystem::rename(temporary,target);
}
career_index save_store::index()const{auto data{read(edition_==game_edition::expansion&&battle_set_?"bar/names.log":"car/names.log")};return data?read_career_index(*data):career_index{};}
void save_store::save_index(career_index const &value)const{write(edition_==game_edition::expansion&&battle_set_?"bar/names.log":"car/names.log",write_career_index(value));}
campaign_start save_store::load(career_entry const &entry)const{
  auto file{read(entry.career_file)},hangars{read(entry.hangar_file)};
  if(!file||!hangars)throw std::runtime_error{"career is missing its saved state or hangar: "+entry.name};
  campaign_start out;out.career=read_career(*file,edition_);binary_reader h{*hangars,"career hangar history"};
  for(auto &roster:out.hangars)roster=read_hangar(h,edition_);
  h.require_end();return out;
}
void save_store::save(career_entry const &entry,campaign_start const &career)const{
  binary_writer h;for(auto const &roster:career.hangars)h.append(write_hangar(roster));
  write(entry.hangar_file,std::move(h).finish());write(entry.career_file,write_career(career.career));
}
void save_store::erase(career_entry const &entry)const{
  std::filesystem::remove(path(entry.career_file));std::filesystem::remove(path(entry.hangar_file));
}
preferences save_store::load_preferences()const{
  if(auto data{read("data/prefs.cfg")})return preferences{*data,true,edition_};
  // Standard sound-enabled initial setup, with original maximum detail.
  const preferences::values defaults{1,1,2,1,255,255,255,0,0,0,0,1,0,0,2,0,0};
  return preferences{defaults,true,edition_};
}
void save_store::save_preferences(preferences const &settings)const{write("data/prefs.cfg",settings.current());}
}
