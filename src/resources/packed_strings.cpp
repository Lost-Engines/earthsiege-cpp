#include "packed_strings.h"
namespace earthsiege {
std::vector<std::string> read_packed_strings(bytes source) {
  binary_reader file{source,"packed strings"};
  auto const count{file.integer<uint32_t>()},amount{file.integer<uint32_t>()};
  if(count>32767)file.fail("excessive string count");
  std::vector<uint16_t> offsets;
  for(unsigned i{};i<count;++i)offsets.push_back(file.integer<uint16_t>());
  auto const pool{file.take(amount)};file.require_end();
  std::vector<std::string> result;
  for(auto offset:offsets){binary_reader text{pool,"packed string"};text.seek(offset);result.push_back(text.cstring());}
  return result;
}
}

namespace earthsiege {
std::string const &shell_text::at(size_t id)const{
 if(edition==game_edition::expansion){
  if(id>=184)id+=14;
  else if(id>=161)id+=13;
  else if(id>=85)id+=9;
  else if(id>=63)id+=5;
  else if(id>=35)id+=1;
 }
 return entries.at(id);
}
std::string const &shell_text::chassis(int16_t id)const{return entries.at(27+human_herc_index(id));}
std::string const &shell_text::weapon(int16_t id,bool campaign)const{
 if(edition==game_edition::expansion){
  if(id==17)return entries.at(46);
  if(id>=18)return entries.at((campaign?46:72)+id);
  return entries.at((campaign||id>=14?47:76)+id);
 }
 return entries.at(id==17?45:static_cast<size_t>((campaign||id>=14?46:71)+id));
}
std::string const &shell_text::weapon_condition(int16_t id)const{return entries.at((edition==game_edition::expansion?152:143)+id);}
std::string const &shell_text::weapon_full(int16_t id)const{return entries.at((edition==game_edition::expansion?227:213)+id);}
}
