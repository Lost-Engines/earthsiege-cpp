#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/artwork.h"
#include "resources/resource_store.h"

auto main(int argc, char *argv[])->int try {
  if(argc != 3) throw std::runtime_error{"expected assets and artwork fixture paths"};
  earthsiege::resource_store const store{argv[1]};
  std::ifstream fixture{argv[2]};
  char kind{};
  std::string volume;
  std::string name;
  unsigned int index{};
  unsigned int width{};
  unsigned int height{};
  uint32_t expected_crc{};
  unsigned int checked{};
  while(fixture >> kind >> volume >> name >> index >> width >> height >> std::hex >> expected_crc >> std::dec) {
    auto const source{store.read(volume, name)};
    std::vector<uint8_t> pixels;
    bool valid{};
    if(kind == 'B') {
      auto const frames{earthsiege::read_bitmaps(source)};
      auto const &frame{frames.at(index)};
      valid = frame.width == width && frame.height == height;
      pixels = frame.pixels;
    } else if(kind == 'P') {
      auto const palette{earthsiege::read_palette(source)};
      valid = palette.colours.size() == width && palette.shade_ramps.size() == height;
      for(auto const colour : palette.colours) {
        for(unsigned int byte{}; byte < 4; ++byte) pixels.push_back(static_cast<uint8_t>(colour >> (byte * 8)));
      }
    } else if(kind == 'F') {
      auto const font{earthsiege::read_font(source)};
      valid = font.first_character == index && font.glyphs.size() == width && font.height == height;
      for(auto const &glyph : font.glyphs) pixels.insert(pixels.end(), glyph.pixels.begin(), glyph.pixels.end());
    }
    if(!valid || earthsiege::crc32(pixels) != expected_crc) throw std::runtime_error{"decoded artwork differs from independent extraction: " + name};
    ++checked;
  }
  if(checked != 4646) throw std::runtime_error{"incomplete artwork fixture"};
  std::cout << checked << " original bitmap, palette and font fixtures agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
