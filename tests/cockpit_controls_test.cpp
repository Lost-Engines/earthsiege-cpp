#include <algorithm>
#include <iostream>
#include "resources/cockpit.h"
#include "resources/input_tape.h"
#include "shell/cockpit_hdd.h"

namespace {
using namespace earthsiege;

void check_layout(binary_reader &file, resource_store const &resources, mission_session &session) {
  auto const name{file.cstring()};
  auto const cockpit{read_cockpit(resources, name)};
  for(auto view : {cockpit_view::forward, cockpit_view::lower, cockpit_view::right, cockpit_view::left}) {
    framebuffer panel;
    cockpit.draw(panel, view);
    if(crc32(panel.pixels()) != file.integer<uint32_t>()) file.fail(name + " view " + std::to_string(std::to_underlying(view)));
  }
  auto const sprites{cockpit.layout.read_buttons(resources)};
  std::vector<uint8_t> sheet;
  for(auto const &sprite : sprites) {
    for(auto dimension : {sprite.width, sprite.height}) {
      sheet.push_back(static_cast<uint8_t>(dimension));
      sheet.push_back(static_cast<uint8_t>(dimension >> 8));
    }
    sheet.insert(sheet.end(), sprite.pixels.begin(), sprite.pixels.end());
  }
  if(crc32(sheet) != file.integer<uint32_t>()) file.fail(name + " instrument sheet");

  pointer_controls input;
  cockpit_hdd hdd{resources, cockpit.layout, session, input, session.interface_random()};
  auto const [ox, oy]{cockpit.windows[1].instrument_origin};
  hdd.update(session, input, session.make_camera(camera_mode::attached), 0, ox, oy);
  std::array<framebuffer, 2> expected;
  for(size_t i{0}; i < 8; ++i) {
    auto const &button{input.at(i)};
    auto const &b{button.bounds};
    for(int coordinate : {b.left, b.top, b.right, b.bottom}) {
      if(coordinate != file.integer<int16_t>()) file.fail(name + " button " + std::to_string(i) + " bounds");
    }
    for(auto &frame : expected) {
      frame.image(sprites.at(file.integer<uint8_t>()), b.left - ox, b.top - oy, true);
    }
    // Isolate each hit area, including any portion outside the painted rectangle.
    pointer_controls isolated;
    isolated.add_button(b, button.diamond);
    std::vector<uint8_t> hits;
    for(int y{b.top - 1}; y <= std::max(b.bottom, b.top + b.right - b.left) + 1; ++y) {
      for(int x{b.left - 1}; x <= b.right + 1; ++x) {
        isolated.reset_input();
        isolated.event(x, y, true, false);
        hits.push_back(isolated.active().has_value());
      }
    }
    if(crc32(hits) != file.integer<uint32_t>()) file.fail(name + " button " + std::to_string(i) + " hit area");
  }
  for(size_t state{0}; state < expected.size(); ++state) {
    // The painter uses the same alternate bitmap for pressed and disabled.
    if(state == 1) for(size_t i{0}; i < 8; ++i) input.enable(i, false);
    framebuffer actual;
    cockpit.draw(actual, cockpit_view::lower);
    hdd.draw(actual, input, -ox, -oy);
    auto const &ink{expected[state].pixels()};
    for(size_t p{0}; p < ink.size(); ++p) {
      if(ink[p] && actual.pixels()[p] != ink[p]) {
        file.fail(name + " button state " + std::to_string(state) + " at " + std::to_string(p % screen_width) + "," + std::to_string(p / screen_width));
      }
    }
  }
}
}

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  auto const bytes{read_file(argv[2])};
  binary_reader file{bytes};
  auto magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCPAD01") file.fail("cockpit fixture version");
  std::filesystem::path root{argv[1]};
  for(auto edition : {game_edition::original, game_edition::expansion}) {
    resource_store resources{edition == game_edition::original ? root : root / "expansion"};
    auto const tape{read_input_tape(read_file(resources.root() / "TAPES/DEMO1.TAP"), edition)};
    auto const &files{tape.starting_files};
    mission_session session{resources, read_mission_script(files[std::to_underlying(tape_file::script)]),
      read_player_roster(files[std::to_underlying(tape_file::player)], edition), random_state{},
      read_mission_counters(files[std::to_underlying(tape_file::campaign_flags)])};
    auto const count{file.integer<uint8_t>()};
    if(count != (edition == game_edition::original ? 8 : 9)) file.fail("chassis count");
    for(unsigned i{0}; i < count; ++i) check_layout(file, resources, session);
  }
  file.require_end();
  std::cout << "68 cockpit views and 136 lower buttons match both original executables\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
