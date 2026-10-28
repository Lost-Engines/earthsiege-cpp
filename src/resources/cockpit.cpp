#include "cockpit.h"
#include <algorithm>
#include <stdexcept>

namespace earthsiege {

screen_mask read_screen_mask(bytes source, std::vector<uint8_t> *unused) {
  binary_reader file{source, "cockpit mask"};
  screen_mask result;
  auto const span{[&](int y, int left, int right){
    left = std::max(0, left);
    right = std::min(static_cast<int>(screen_width) - 1, right);
    if(y >= 0 && y < static_cast<int>(screen_height) && left <= right) result.rows[static_cast<size_t>(y)].push_back({left, right});
  }};
  auto const rectangles{file.integer<uint16_t>()};
  for(unsigned int i{}; i < rectangles; ++i) {
    int const top{file.integer<int16_t>()}, bottom{file.integer<int16_t>()};
    if(bottom < top) file.fail("inverted clipping rectangle");
    screen_region region{.first_row{top}, .spans{}, .rectangular{true}};
    for(int y{top}; y <= bottom; ++y) { span(y, 0, screen_width - 1); region.spans.push_back({0, screen_width - 1}); }
    result.regions.push_back(std::move(region));
  }
  auto const blocks{file.integer<uint16_t>()};
  for(unsigned int i{}; i < blocks; ++i) {
    int const first{file.integer<int16_t>()}, count{file.integer<int16_t>()};
    if(count < 0) file.fail("negative span count");
    screen_region region{.first_row{first}, .spans{}, .rectangular{false}};
    for(int row{}; row < count; ++row) {
      int const left{file.integer<int16_t>()}, right{file.integer<int16_t>()};
      span(first + row, left, right);
      region.spans.push_back({left, right});
    }
    result.regions.push_back(std::move(region));
  }
  // COLOSSUS.ED1 includes legacy trailing data which the original reader ignores.
  if(unused) { auto const rest{file.take(file.remaining())}; unused->assign(rest.begin(), rest.end()); }
  for(auto &row : result.rows) {
    std::ranges::sort(row, {}, &scanline_span::left);
    std::vector<scanline_span> joined;
    for(auto const value : row) {
      if(!joined.empty() && value.left <= joined.back().right + 1) joined.back().right = std::max(joined.back().right, value.right);
      else joined.push_back(value);
    }
    row = std::move(joined);
  }
  return result;
}

unsigned int cockpit_layout::weapon_count() const { return static_cast<unsigned int>(words[4]); }
rectangle cockpit_layout::bounds(size_t byte_offset) const {
  if(byte_offset % 4) throw std::invalid_argument{"unaligned cockpit layout field"};
  size_t const i{byte_offset / 4};
  return {words.at(i), words.at(i + 1), words.at(i + 2), words.at(i + 3)};
}

cockpit_layout read_cockpit_layout(bytes source) {
  binary_reader file{source, "cockpit layout"};
  cockpit_layout result;
  for(auto &value : result.words) value = file.integer<int32_t>();
  file.require_end();
  if(result.words[4] < 0 || result.words[4] > 9) file.fail("invalid cockpit weapon count");
  auto const rebase{[&](size_t offset){
    for(size_t i{}; i < 4; ++i) result.words[offset / 4 + i] += result.words[i % 2];
  }};
  for(unsigned int i{}; i < result.weapon_count(); ++i) rebase(20 + i * 16);
  for(size_t const offset : {0x1c4, 0x224, 0x268, 0x2d8, 0x3e8, 0x450, 0x4bc}) rebase(offset);
  return result;
}

std::array<cockpit_window, 4> read_cockpit_windows(bytes source) {
  binary_reader file{source, "cockpit viewports"};
  if(file.integer<uint32_t>() != 4) file.fail("expected four cockpit views");
  std::array<cockpit_window, 4> result;
  for(auto &view : result) {
    view.bounds = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    for(auto &value : view.projection_origin) value = file.integer<int32_t>();
    for(auto &value : view.instrument_origin) value = file.integer<int32_t>();
  }
  file.require_end();
  return result;
}

cockpit_artwork read_cockpit(resource_store const &resources, std::string const &name) {
  cockpit_artwork result;
  result.layout = read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"));
  result.windows = read_cockpit_windows(resources.read("simvol0", "vue/" + name + ".VUE"));
  for(unsigned int i{}; i < 4; ++i) {
    auto const suffix{"ED" + std::to_string(i)};
    result.windows[i].mask = read_screen_mask(resources.read("simvol0", suffix + "/" + name + "." + suffix), &result.windows[i].unused_mask_bytes);
  }
  for(unsigned int i{}; i < 3; ++i) {
    auto const suffix{"DB" + std::to_string(i)};
    auto bank{read_bitmaps(resources.read("simvol0", suffix + "/" + name + "." + suffix))};
    if(bank.size() != 1) throw std::runtime_error{"cockpit panel must have one bitmap"};
    result.panels[i] = std::move(bank.front());
  }
  return result;
}

void cockpit_artwork::draw(framebuffer &target, cockpit_view view) const {
  scoped_framebuffer_clip unclipped{target, nullptr};
  auto const index{static_cast<unsigned int>(view)};
  auto const &panel{panels.at(std::min(index, 2U))};
  if(view != cockpit_view::left) target.image(panel, 0, 0);
  else for(unsigned int y{}; y < panel.height; ++y) for(unsigned int x{}; x < panel.width; ++x)
    target.pixel(static_cast<int>(x), static_cast<int>(y), panel.pixels[y * panel.width + panel.width - 1 - x]);
}

void apply_cockpit_colours(palette &world, palette const &cockpit, unsigned int chassis) {
  if(chassis >= 8) throw std::out_of_range{"cockpit chassis index"};
  auto const copy{[&](unsigned int target, unsigned int source){
    world.colours[target] = cockpit.colours[source];
    world.dac_colours[target] = cockpit.dac_colours[source];
  }};
  for(unsigned int i{}; i < 16; ++i) { copy(32 + i, chassis * 32 + i); copy(64 + i, chassis * 32 + 16 + i); }
  constexpr std::array<unsigned int, 8> highlights{44, 37, 45, 39, 45, 45, 45, 40};
  world.colours[64] = world.colours[highlights[chassis]];
  world.dac_colours[64] = world.dac_colours[highlights[chassis]];
}

} // namespace earthsiege
