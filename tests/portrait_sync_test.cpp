#include "resources/portrait_sync.h"
#include "resources/resource_store.h"
#include <cstdlib>
#include <iostream>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESSNC001")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};size_t total{};
  for(uint32_t i{};i<count;++i){
    auto const raw{file.take(file.integer<uint16_t>())};std::string const name{raw.begin(),raw.end()};auto const ticks{file.integer<uint32_t>()};
    std::vector<int16_t> expected;for(uint32_t t{};t<ticks;++t)expected.push_back(file.integer<int16_t>());
    portrait_animation animation{read_portrait_sync(resources.read("simvol0","snc/"+name+".SNC"))};animation.start();
    for(uint32_t t{};t<ticks;++t){
      animation.advance(t);auto const deadline{file.integer<uint32_t>()};
      if(animation.frame()!=expected[t] || animation.next_tick()!=deadline)file.fail(name+" at tick "+std::to_string(t));
      ++total;
    }
  }
  file.require_end();std::cout<<count<<" original SNC animations and "<<total<<" frame/deadline states match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
