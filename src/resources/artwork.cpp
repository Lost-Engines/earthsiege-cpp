#include "artwork.h"
#include <algorithm>
#include <limits>

namespace earthsiege {

namespace {

auto read_bitmap(binary_reader &file)->bitmap {
  auto const start{file.position()};
  auto const tag{file.integer<uint32_t>()};
  auto const size{file.integer<uint32_t>()};
  bitmap result{
    .width{},
    .height{file.integer<uint16_t>()},
    .flags{},
    .pixels{},
  };
  result.width = file.integer<uint16_t>();
  unsigned int bits{};
  uint32_t amount{};
  if(tag == 0x280000) {
    bits = file.integer<uint16_t>();
    result.flags = static_cast<uint8_t>(file.integer<uint16_t>());
    amount = file.integer<uint32_t>();
  } else if(tag == 0x28000e) {
    bits = file.integer<uint8_t>();
    result.flags = file.integer<uint8_t>();
    file.take(1);
    amount = file.integer<uint32_t>();
    file.take(2);
  } else file.fail("unknown bitmap tag");
  if(bits != 8 || !result.width || !result.height || amount != result.width * result.height) {
    file.fail("unsupported bitmap raster");
  }
  auto const raster{file.take(amount)};
  result.pixels.assign(raster.begin(), raster.end());
  size_t const end{start + 8 + size};
  if(file.position() > end) file.fail("bitmap exceeds tagged extent");
  // the legacy ICONS.DBA declares stale cached bytes beyond its complete raster
  if(end > file.position() + file.remaining()) {
    if(tag != 0x280000) file.fail("bitmap extent exceeds resource");
  } else file.seek(end + ((end & 1) && end < file.position() + file.remaining() ? 1 : 0));
  return result;
}

} // namespace

auto read_bitmaps(bytes data)->std::vector<bitmap> {
  /// Decode DBM, DBA and DCI objects, retaining complete indexed pixel data.
  binary_reader file{data, "bitmap resource"};
  auto const tag{file.integer<uint32_t>()};
  unsigned int count{1};
  if(tag == 0x280001) {
    file.take(4);
    count = file.integer<uint32_t>();
  } else if(tag == 0x28000b) file.seek(16);
  else file.seek(0);
  if(count > 10000) file.fail("excessive bitmap array count");
  std::vector<bitmap> result;
  result.reserve(count);
  for(unsigned int index{}; index < count; ++index) result.push_back(read_bitmap(file));
  return result;
}

auto display_colour(std::array<uint8_t,3> const &dac)->uint32_t {
  auto expand=[](uint8_t c)->uint32_t{return (static_cast<uint32_t>(c)<<2)|(c>>4);};
  return 0xff000000u|(expand(dac[0])<<16)|(expand(dac[1])<<8)|expand(dac[2]);
}

auto read_palette(bytes data)->palette {
  /// Decode VGA DAC colours and the original discrete shade ramps.
  binary_reader file{data, "palette"};
  auto const tag{file.integer<uint32_t>()};
  if(tag != 0x28000f && tag != 0x280002) file.fail("unknown palette tag");
  file.take(4);
  auto const count{file.integer<uint32_t>()};
  if(count > 256) file.fail("excessive palette colours");
  palette result;
  for(unsigned int index{}; index < count; ++index) {
    unsigned int const red{file.integer<uint8_t>()};
    unsigned int const green{file.integer<uint8_t>()};
    unsigned int const blue{file.integer<uint8_t>()};
    file.take(1);
    if(red > 63 || green > 63 || blue > 63) file.fail("colour exceeds VGA DAC range");
    result.dac_colours[index] = {static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue)};
    result.colours[index] = display_colour(result.dac_colours[index]);
  }
  if(file.remaining()) {
    auto const ramps{file.integer<uint32_t>()};
    if(ramps > 256) file.fail("excessive palette ramps");
    for(unsigned int index{}; index < ramps; ++index) {
      auto const length{file.integer<uint16_t>()};
      std::vector<uint16_t> ramp;
      ramp.reserve(length);
      for(unsigned int level{}; level < length; ++level) ramp.push_back(file.integer<uint16_t>());
      result.shade_ramps.push_back(std::move(ramp));
    }
  }
  file.require_end();
  return result;
}

auto read_font(bytes data)->font {
  /// Read original DFN glyph widths, offsets and palette-indexed rasters.
  binary_reader file{data, "font"};
  if(file.integer<uint32_t>() != 0x280005) file.fail("unknown font tag");
  file.take(4);
  auto const count{file.integer<uint32_t>()};
  font result{
    .first_character{file.integer<uint16_t>()},
    .height{file.integer<uint16_t>()},
    .baseline{},
    .glyphs{},
  };
  if(count > 256 || result.height > 256) file.fail("excessive font dimensions");
  file.seek(26);
  result.baseline = file.integer<int16_t>();
  file.seek(30);
  auto const raster_size{file.integer<uint32_t>()};
  auto const raster{file.take(raster_size)};
  binary_reader offsets{file.take(count * 4), "glyph offsets"};
  auto const widths{file.take(count)};
  for(unsigned int index{}; index < count; ++index) {
    auto const offset{offsets.integer<uint32_t>()};
    size_t const amount{widths[index] * result.height};
    if(offset > raster.size() || amount > raster.size() - offset) file.fail("glyph outside font raster");
    auto const pixels{raster.subspan(offset, amount)};
    result.glyphs.push_back(glyph{
      .width{widths[index]},
      .pixels{pixels.begin(), pixels.end()},
    });
  }
  file.require_end();
  return result;
}

auto font::text_width(std::string_view text) const->int {
  int width{};
  for(unsigned char const character : text) {
    if(character >= first_character && character - first_character < glyphs.size()) {
      width += static_cast<int>(glyphs[character - first_character].width);
    }
  }
  return width;
}

} // namespace earthsiege
