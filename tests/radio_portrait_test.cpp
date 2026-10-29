#include "render/radio_portrait.h"
#include <cstdlib>
#include <iostream>
#include <memory>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader file{data};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESRPRT01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};std::unique_ptr<radio_portrait> portrait;int previous{-1};bool last{};
  for(uint32_t i{};i<count;++i){
    auto const pilot{file.integer<int32_t>()};bool const reduced{file.integer<int32_t>()!=0};auto const mouth{file.integer<int32_t>()},seed{file.integer<int32_t>()};auto const crc{file.integer<uint32_t>()};
    if(pilot!=previous || reduced!=last){portrait=std::make_unique<radio_portrait>(resources,static_cast<size_t>(pilot),reduced);previous=pilot;last=reduced;}
    random_state random{static_cast<int16_t>(seed)};portrait->update(static_cast<size_t>(mouth),random);framebuffer frame;frame.clear(123);portrait->draw(frame,{20,40,76,89});
    if(crc32(frame.pixels())!=crc)file.fail("portrait pixels "+std::to_string(i));
    for(auto word:random.snapshot().words)if(word!=file.integer<uint16_t>())file.fail("portrait RNG words");
    if(random.snapshot().first!=file.integer<uint8_t>() || random.snapshot().second!=file.integer<uint8_t>())file.fail("portrait RNG indices");
  }
  file.require_end();std::cout<<count<<" original portrait rasters and UI RNG states match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
