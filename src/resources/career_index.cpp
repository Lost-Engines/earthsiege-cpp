#include "career_index.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
namespace {
std::string fixed_string(binary_reader &f,size_t size){auto b{f.take(size)};auto end{std::ranges::find(b,uint8_t{})};if(end==b.end())f.fail("unterminated career index string");return {b.begin(),end};}
void fixed_string(binary_writer &f,std::string const &s,size_t size){if(s.size()>=size)throw std::runtime_error{"career index field too long"};for(char c:s)f.integer<uint8_t>(static_cast<uint8_t>(c));for(size_t i{s.size()};i<size;++i)f.integer<uint8_t>(0);}
}
career_index read_career_index(bytes source){
  binary_reader f{source,"career index"};career_index out;auto count{f.integer<int16_t>()};if(count<0)f.fail("negative career count");
  if(count){out.selected=f.integer<int16_t>();if(out.selected<0||out.selected>=count)f.fail("invalid selected career");}
  for(int i{};i<count;++i)out.entries.push_back({fixed_string(f,64),f.integer<int16_t>(),fixed_string(f,17),fixed_string(f,17)});
  f.require_end();return out;
}
std::vector<uint8_t> write_career_index(career_index const &index){
  if(index.entries.size()>32767)throw std::runtime_error{"too many careers"};
  binary_writer f;f.integer<int16_t>(static_cast<int16_t>(index.entries.size()));
  if(!index.entries.empty()){if(index.selected<0||static_cast<size_t>(index.selected)>=index.entries.size())throw std::runtime_error{"invalid selected career"};f.integer(index.selected);}
  for(auto const &e:index.entries){fixed_string(f,e.name,64);f.integer(e.portrait);fixed_string(f,e.career_file,17);fixed_string(f,e.hangar_file,17);}
  return std::move(f).finish();
}
}
