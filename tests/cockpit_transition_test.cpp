#include <cstdlib>
#include <iostream>
#include "resources/resource_store.h"
#include "shell/cockpit_transition.h"
using earthsiege::binary_reader;
using earthsiege::cockpit_transition;
using earthsiege::read_file;
int main(int argc,char **argv)try{
  if(argc != 2)return EXIT_FAILURE;
  auto raw{read_file(argv[1])};binary_reader file{raw};auto magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESVIEW01")file.fail("fixture version");
  auto read=[&]{cockpit_transition::state s;
    s.view = file.integer<int32_t>();s.pending = file.integer<int32_t>();
    s.prepared = file.integer<int32_t>();s.cooldown = file.integer<int32_t>();
    s.saved_view = file.integer<int32_t>();s.draw_page = file.integer<int32_t>();s.other_page = file.integer<int32_t>();return s;};
  cockpit_transition policy{read()};auto count{file.integer<uint32_t>()};
  for(unsigned i{};i < count;++i){
    auto request{file.integer<int32_t>()};auto before{read()},after{read()};
    if(policy.snapshot() != before)file.fail("initial state " + std::to_string(i));
    policy.request(request);policy.present();
    if(policy.snapshot() != after)file.fail("presentation state " + std::to_string(i));
  }
  file.require_end();std::cout << count << " original view presentations agree\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr << e.what() << '\n';return EXIT_FAILURE;}
