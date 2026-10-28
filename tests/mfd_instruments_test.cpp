#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include "render/mfd_instruments.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMFDUI1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis; std::unique_ptr<mfd_instruments> display;
  for(uint32_t i{}; i < count; ++i) {
    auto const name{file.cstring()};
    if(chassis != name) {
      chassis = name;
      display = std::make_unique<mfd_instruments>(resources, read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU")));
    }
    auto const x{file.integer<int32_t>()}, y{file.integer<int32_t>()};
    auto const page{static_cast<mfd_page>(file.integer<uint8_t>())};
    auto const enabled{mfd_enabled_buttons(page)};
    for(auto const value : enabled) if(value != bool(file.integer<uint8_t>())) file.fail("MFD enabled buttons differ");
    std::array<unsigned int, mfd_button_count> faces;
    for(auto &face : faces) face = file.integer<uint8_t>();
    auto const check_bounds{[&](rectangle const &bounds) {
      if(file.integer<int32_t>() != bounds.left + x || file.integer<int32_t>() != bounds.top + y ||
         file.integer<int32_t>() != bounds.right + x || file.integer<int32_t>() != bounds.bottom + y)
        file.fail(name + " MFD control bounds differ");
    }};
    for(auto const &bounds : display->button_bounds()) check_bounds(bounds);
    check_bounds(display->display_bounds());
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame; frame.clear(123); display->draw(frame, page, faces, x, y);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-mfd-controls-difference.raw", std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail(name + " MFD button paint differs at " + std::to_string(i));
    }
  }
  auto const ranges{file.integer<uint32_t>()};
  for(uint32_t i{}; i < ranges; ++i) {
    auto const before{file.integer<int32_t>()}, returned{file.integer<int32_t>()}, after{file.integer<int32_t>()}, distance{file.integer<int32_t>()};
    if(static_cast<int32_t>(next_mfd_range(static_cast<size_t>(before))) != after || returned != after || mfd_ranges.at(static_cast<size_t>(after)) != distance)
      file.fail("MFD range cycle differs");
  }
  file.require_end(); std::cout << count << " original MFD button frames and " << ranges << " range cycles match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
