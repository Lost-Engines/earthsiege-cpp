#include <cstdlib>
#include <iostream>
#include "shell/tactical_controller.h"
#include "resources/resource_store.h"
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return EXIT_FAILURE;
  auto const data{read_file(argv[1])};binary_reader file{data};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESTTGT01")file.fail("fixture version");
  rectangle const bounds{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
  auto const count{file.integer<uint32_t>()};unsigned failures{};
  for(unsigned i{};i<count;++i){
    vec3<int32_t> const camera{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()},player{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    auto const side{file.integer<uint8_t>()};int const previous{file.integer<int32_t>()},expected{file.integer<int32_t>()};
    auto const n{file.integer<uint16_t>()};std::vector<tactical_contact> contacts;
    for(unsigned j{};j<n;++j){tactical_contact c;c.actor=j;c.position={file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};c.side=file.integer<uint8_t>();c.dead=file.integer<uint8_t>();c.selectable=file.integer<uint8_t>();c.detected=file.integer<uint8_t>();c.known=file.integer<uint8_t>();contacts.push_back(c);}
    auto const result{cycle_tactical_target(contacts,camera,bounds,player,side,previous<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(previous)})};
    int const actual{result?static_cast<int>(*result):-1};if(actual!=expected){++failures;if(failures<10)std::cerr<<"target cycle "<<i<<": "<<actual<<" != "<<expected<<'\n';}
  }
  file.require_end();if(failures)return EXIT_FAILURE;
  std::cout<<count<<" original tactical keyboard target cycles match\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
