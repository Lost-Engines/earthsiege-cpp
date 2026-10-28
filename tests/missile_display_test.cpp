#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/missile_display.h"
#include "resources/resource_store.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMIS001") file.fail("fixture version");
  std::array<int16_t, 10> header;
  for(auto &value : header) value = file.integer<int16_t>();
  mission_header const mission{.terrain{static_cast<terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
  scenery_world world{resources, mission}; framebuffer frame;
  missile_display display{resources, frame, world}; missile_camera_tracker tracker;
  auto const slot{[&]()->std::optional<size_t>{ auto const value{file.integer<int32_t>()}; return value < 0 ? std::nullopt : std::optional<size_t>{value}; }};
  auto const position{[&]{ return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; }};
  auto const angles{[&]{ return vec3<uint16_t>{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()}; }};
  auto const count{file.integer<uint32_t>()};
  unsigned int failures{};
  for(unsigned int i{}; i < count; ++i) {
    rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    tracker.watch(slot()); bool const ready{file.integer<uint8_t>() != 0}, locked{file.integer<uint8_t>() != 0};
    auto const now{file.integer<uint32_t>()};
    std::vector<missile_camera_object> missiles(file.integer<uint16_t>());
    for(auto &m : missiles) { m.id = file.integer<uint32_t>(); m.position = position(); m.angles = angles(); }
    std::vector<missile_camera_explosion> explosions(file.integer<uint16_t>());
    for(auto &e : explosions) { e.id = file.integer<uint32_t>(); e.position = position(); }
    auto const kind{static_cast<missile_picture>(file.integer<uint8_t>())};
    auto const watched{slot()}, explosion{slot()}; bool const bright{file.integer<uint8_t>() != 0};
    auto const retained_position{position()}; auto const retained_angles{angles()}; auto const crc{file.integer<uint32_t>()};
    auto const state{tracker.advance(missiles, explosions)};
    bool const same{state.picture == kind && state.position == retained_position && state.angles == retained_angles &&
                    state.bright == bright && tracker.watched() == watched && tracker.flashing_explosion() == explosion};
    frame.clear(123); display.draw(bounds, state, ready, locked, now, std::array{default_world_light()});
    bool const raster{crc32(frame.pixels()) == crc};
    if(!same || !raster) {
      if(!failures) {
        std::ofstream output{"/tmp/es-missile-difference.raw", std::ios::binary};
        output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      }
      ++failures; std::cerr << "missile frame " << i << " state " << same << " raster " << raster << '\n';
    }
  }
  file.require_end();
  if(failures) file.fail(std::to_string(failures) + " missile display frames differ");
  std::cout << count << " original missile display frames and states match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
