#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "shell/mission_alert.h"
using earthsiege::binary_reader;
using earthsiege::framebuffer;
using earthsiege::mission_alert;
using earthsiege::read_file;
using earthsiege::read_palette;
using earthsiege::resource_store;
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto data{read_file(argv[2])};
  binary_reader f{data};
  auto const version{f.take(8)};
  if(std::string(version.begin(), version.end()) != "ESALT002") throw std::runtime_error{"alert fixture version"};
  auto const count{f.integer<uint32_t>()};
  for(unsigned i{0}; i < count; ++i) {
    auto const message{f.integer<int16_t>()};
    auto before{f.take(64000)};
    auto expected{f.take(64000)};
    framebuffer target;
    for(int y{0}; y < 200; ++y) {
      for(int x{0}; x < 320; ++x) target.pixel(x, y, before[y * 320 + x]);
    }
    mission_alert alert{resources, message};
    alert.draw(target);
    int mismatch{0};
    for(int y{0}; y < 200; ++y) {
      for(int x{0}; x < 320; ++x) {
        if(target.pixels()[y * 320 + x] != expected[y * 320 + x]) ++mismatch;
      }
    }
    if(mismatch) {
      std::ofstream raw{"/tmp/es-alert-native.raw", std::ios::binary};
      raw.write(reinterpret_cast<char const *>(target.pixels().data()), 64000);
      raw.close();
      target.save("/tmp/es-alert-native.ppm", read_palette(resources.read("shell0", "res32/regn1000.dpl")));
      std::cerr << "message " << message << ": " << mismatch << " mismatched alert pixels\n";
      return EXIT_FAILURE;
    }
    if(alert.key(1) != 0) throw std::runtime_error{"Escape must continue"};
    if(alert.count() == 2) {
      alert.key(15);
      if(alert.key(28) != 1) throw std::runtime_error{"Tab then Enter confirms second response"};
    }
  }
  f.require_end();
  std::cout << count << " original modal alert rasters match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
