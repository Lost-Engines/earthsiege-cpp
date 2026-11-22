#include <cstdlib>
#include <iostream>
#include "render/shape_preferences.h"
#include "resources/binary_reader.h"
using earthsiege::binary_reader;
using earthsiege::read_file;
using earthsiege::shape_detail;
int main(int argc,char **argv)try{
 if(argc != 2)return EXIT_FAILURE;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto m{f.take(8)};
 if(std::string(m.begin(),m.end()) != "ESSHPR01")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i < count;++i){
  bool reduced{f.integer<uint8_t>() != 0};auto value{f.integer<uint8_t>()};
  auto s{shape_detail(value,reduced)};
  if(f.integer<int16_t>() != s.actor.scale || static_cast<unsigned>(f.integer<int16_t>()) != s.actor.bias || static_cast<unsigned>(f.integer<int32_t>()) != s.ordinary || static_cast<unsigned>(f.integer<int32_t>()) != s.weapon || static_cast<unsigned>(f.integer<int32_t>()) != s.herc)f.fail("Shapes preference mismatch");
 }
 f.require_end();std::cout << count << " original full/low-memory Shapes settings agree\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr << e.what() << '\n';return EXIT_FAILURE;}
