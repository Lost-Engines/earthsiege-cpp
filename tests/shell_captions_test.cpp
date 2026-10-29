#include "render/shell_caption.h"
#include "resources/packed_strings.h"
#include <fstream>
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader f{raw};
 auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESCAPTN1")f.fail("fixture version");
 auto cases{f.integer<uint32_t>()};unsigned frames{};
 for(unsigned c{};c<cases;++c){
  auto name{f.cstring()};auto index{f.integer<uint16_t>()},region{f.integer<uint16_t>()};
  rectangle bounds{f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>(),f.integer<int32_t>()};
  auto count{f.integer<uint16_t>()},pages{f.integer<uint16_t>()},per_page{f.integer<uint16_t>()};
  auto data{resources.read("shell0","eng/"+name)};auto strings{read_packed_strings(bytes{data}.subspan(2))};
  shell_caption caption{resources,strings.at(index),region};
  if(caption.lines().size()!=count||caption.page_count()!=pages||caption.page_rows()!=per_page)f.fail(name+" original caption row/page counts");
  auto const b{caption.bounds()};
  if(b.left!=bounds.left||b.top!=bounds.top||b.right!=bounds.right||b.bottom!=bounds.bottom)f.fail("caption bounds");
  for(unsigned i{};i<count;++i){
   std::optional<std::string> line;if(f.integer<uint8_t>())line=f.cstring();
   if(caption.lines()[i]!=line)f.fail(name+"/"+std::to_string(index)+" original caption line "+std::to_string(i));
  }
  auto events{f.integer<uint16_t>()};
  for(unsigned e{};e<events;++e){
   auto action{f.integer<int8_t>()};auto preference{f.integer<uint8_t>()};auto page{f.integer<uint16_t>()};auto crc{f.integer<uint32_t>()};
   if(action>0)caption.next();else if(action<0)caption.previous();
   if(caption.page()!=page)f.fail("caption page selection");
   framebuffer target;target.clear(37);caption.draw(target,preference);
   if(crc32(target.pixels())!=crc){
    std::ofstream dump{"build/caption-actual.raw",std::ios::binary};auto const &p{target.pixels()};dump.write(reinterpret_cast<char const *>(p.data()),p.size());
    f.fail(name+"/"+std::to_string(index)+"/"+std::to_string(e)+" original caption pixels");
   }++frames;
  }
 }
 f.require_end();std::cout<<cases<<" original captions / "<<frames<<" original frames agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
