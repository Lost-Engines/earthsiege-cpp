#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "render/weapon_instruments.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWPNI01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; unsigned int failed{};
  for(unsigned int i{}; i < count; ++i) {
    auto const chassis{file.cstring()}, name{file.cstring()};
    auto const kind{static_cast<weapon_instrument_kind>(file.integer<uint8_t>())};
    auto const slot{file.integer<uint8_t>()};
    weapon_instrument_state state;
    state.value = file.integer<int32_t>(); state.requested_charge = file.integer<int32_t>();
    state.disabled = file.integer<uint8_t>(); state.selected = file.integer<uint8_t>();
    state.included = file.integer<uint8_t>(); state.ready = file.integer<uint8_t>();
    auto const face{file.integer<uint8_t>()}; file.take(1);
    auto const expected{file.integer<uint32_t>()};
    rectangle const button{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + chassis + ".GAU"))};
    weapon_instruments instruments{resources, layout};
    auto const bounds{instruments.button_bounds(slot, kind)};
    if(bounds.left != button.left || bounds.top != button.top || (!state.disabled && (bounds.right != button.right || bounds.bottom != button.bottom)))
      file.fail("weapon input bounds differ");
    framebuffer frame; frame.clear(123); instruments.draw(frame, slot, kind, name, state, face);
    if(crc32(frame.pixels()) != expected) {
      ++failed;
      if(failed < 20) std::cerr << i << ' ' << chassis << ' ' << static_cast<int>(kind) << " slot " << static_cast<int>(slot) << " value " << state.value << " requested " << state.requested_charge << " disabled " << state.disabled << " differs: " << crc32(frame.pixels()) << " expected " << expected << '\n';
      if(failed == 1) {
        std::ofstream diagnostic{"/tmp/earthsiege-weapon-actual.raw",std::ios::binary};
        diagnostic.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      }
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " weapon frames differ"};
  std::cout << count << " original weapon instrument frames and input bounds match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
