#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/cockpit.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCOCK02") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const palette_bank{read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL"))};
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()}; auto const chassis{file.integer<uint32_t>()};
    auto const cockpit{read_cockpit(resources, name)};
    for(auto value : cockpit.layout.words) if(value != file.integer<int32_t>()) file.fail(name + " layout differs");
    for(unsigned int view{}; view < 4; ++view) {
      auto const &w{cockpit.windows[view]};
      for(auto value : {w.bounds.left,w.bounds.top,w.bounds.right,w.bounds.bottom,w.projection_origin[0],w.projection_origin[1],w.instrument_origin[0],w.instrument_origin[1]})
        if(value != file.integer<int32_t>()) file.fail(name + " viewport differs");
      framebuffer frame;
      { scoped_framebuffer_clip clip{frame, &w.mask}; cockpit.draw(frame, static_cast<cockpit_view>(view)); }
      if(frame.clip()) file.fail("clip scope leaked");
      if(crc32(frame.pixels()) != file.integer<uint32_t>()) file.fail(name + " bitmap/mirroring differs");
      for(auto value : w.projection_centre()) if(value != file.integer<int32_t>()) file.fail(name + " projection centre differs");
    }
    auto world{read_palette(resources.read("simvol0", "dpl/WORLD0.DPL"))};
    apply_cockpit_colours(world, palette_bank, chassis);
    for(unsigned int offset : {32U,64U}) for(unsigned int j{}; j < 16; ++j)
      if(world.colours[offset+j] != file.integer<uint32_t>()) file.fail(name + " chassis palette differs");
  }
  file.require_end();
  std::cout << count << " complete cockpit layouts, palette banks and 32 artwork views match original readers and selectors\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
