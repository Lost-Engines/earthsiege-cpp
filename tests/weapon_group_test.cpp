#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "render/weapon_instruments.h"
#include "simulation/player_armament.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWPNG01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()}; auto const group{file.integer<uint8_t>()};
    std::array<unsigned int, 5> faces; for(auto &face : faces) face = file.integer<uint8_t>();
    auto const expected{file.integer<uint32_t>()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    weapon_instruments instruments{resources,layout};
    for(auto const &bounds : instruments.group_bounds()) for(auto const value : {bounds.left,bounds.top,bounds.right,bounds.bottom})
      if(value != file.integer<int32_t>()) file.fail("group control rectangle differs");
    framebuffer frame; frame.clear(123); instruments.draw_groups(frame,group,faces);
    if(crc32(frame.pixels()) != expected) file.fail(name + " group paint differs: " + std::to_string(crc32(frame.pixels())) + " expected " + std::to_string(expected));
  }
  auto const inputs{file.integer<uint32_t>()};
  for(unsigned int i{}; i < inputs; ++i) {
    unsigned int const group{file.integer<uint8_t>()}, requested{file.integer<uint8_t>()}, keyboard{file.integer<uint8_t>()};
    bool const tracking{file.integer<uint8_t>() != 0}, wanted_tracking{file.integer<uint8_t>() != 0}, link{file.integer<uint8_t>() != 0};
    std::array<unsigned int, 6> expected; for(auto &value : expected) value = file.integer<uint8_t>();
    weapon_catalogue definitions{}; model_resource models;
    weapon_bank bank{definitions,models,{},{}};
    player_armament controls{bank,definitions,{}};
    controls.change_group(static_cast<uint8_t>(keyboard ? (group + 2) % 3 : group));
    controls.set_tracking(tracking);
    controls.request_group_control(requested);
    if(tracking != wanted_tracking) controls.request_group_control(4);
    if(link) controls.request_group_control(3);
    if(keyboard) controls.command(48);
    if(controls.current_group() != group || controls.auto_tracking() != tracking) file.fail("group UI applied before exchange");
    controls.exchange_group_controls();
    if(controls.current_group() != expected[0] || controls.auto_tracking() != bool(expected[1])) file.fail("group exchange differs");
    if(expected[2] || expected[5] || expected[3] != expected[0] || expected[4] != expected[1]) file.fail("original exchange did not consume requests");
    // A new mouse selection must work after the keyboard latch was consumed.
    controls.request_group_control((expected[0] + 1) % 3); controls.exchange_group_controls();
    if(controls.current_group() != (expected[0] + 1) % 3) file.fail("keyboard precedence persisted into the following update");
  }
  file.require_end();
  std::cout << count << " original group frames and " << inputs << " group exchanges match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
