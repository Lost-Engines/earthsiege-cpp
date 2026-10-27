#include "gouraud_raster.h"
#include <algorithm>
#include <stdexcept>
#include "render/palette_shading.h"
#include "render/polygon_raster.h"
#include "simulation/fixed_math.h"

namespace earthsiege {

std::vector<palette_band> split_gouraud_triangle(std::array<vec2<int32_t>, 3> const &points, std::array<uint8_t, 3> const colours) {
  /// Original Gouraud shading emits solid palette bands, not RGB interpolation
  std::vector<palette_band> result;
  if(points[0] == points[1] || points[0] == points[2] || points[1] == points[2]) return result;
  auto const [minimum, maximum]{std::ranges::minmax(colours)};
  if(minimum == maximum) {
    palette_band band{.vertex_count{3}, .colour{maximum}};
    std::ranges::copy(points, band.points.begin());
    result.push_back(band);
    return result;
  }
  unsigned int high{0};
  unsigned int low{0};
  for(unsigned int index{0}; index != 3; ++index) {
    if(colours[index] == maximum) high = index;                                 // native ties select the last vertex
    if(colours[index] == minimum) low = index;
  }
  auto const middle{3U - high - low};
  unsigned int const upper_steps{static_cast<unsigned int>(maximum) - colours[middle]};
  unsigned int const lower_steps{static_cast<unsigned int>(colours[middle]) - minimum};
  unsigned int const count{static_cast<unsigned int>(maximum) - minimum};
  auto const edge{[&](unsigned int const from, unsigned int const to, unsigned int const steps){
    std::vector<vec2<int32_t>> samples;
    samples.reserve(steps + 1);
    samples.push_back(points[from]);
    for(unsigned int index{1}; index <= steps; ++index) {
      vec2<int32_t> point;
      for(unsigned int axis{0}; axis != 2; ++axis) {
        auto const difference{wrap_long(static_cast<int64_t>(points[to][axis]) - points[from][axis])};
        auto const product{wrap_long(static_cast<int64_t>(difference) * index)};
        point[axis] = wrap_long(static_cast<int64_t>(points[from][axis]) + product / static_cast<int32_t>(steps));
      }
      samples.push_back(point);
    }
    return samples;
  }};
  auto const direct{edge(high, low, count)};
  std::vector<vec2<int32_t>> bent;
  if(upper_steps == 0) bent = edge(middle, low, count);
  else if(lower_steps == 0) bent = edge(high, middle, count);
  else {
    bent = edge(high, middle, upper_steps);
    bent.pop_back();
    auto const lower{edge(middle, low, lower_steps)};
    bent.insert(bent.end(), lower.begin(), lower.end());
  }
  bool const flip{(high + 1) % 3 != low};
  result.reserve(count);
  for(unsigned int index{0}; index != count; ++index) {
    auto const &a{flip ? bent : direct};
    auto const &b{flip ? direct : bent};
    palette_band band{.vertex_count{4}, .colour{static_cast<uint8_t>(maximum - index)}};
    band.points[0] = a[index];
    band.points[1] = a[index + 1];
    band.points[2] = b[index + 1];
    band.points[3] = b[index];
    result.push_back(band);
  }
  return result;
}

void draw_gouraud_triangle(framebuffer &target, std::array<vec2<int32_t>, 3> const &points, std::array<uint8_t, 3> const colours, rectangle const bounds) {
  for(auto const &band : split_gouraud_triangle(points, colours)) draw_solid_polygon(target, std::span{band.points}.first(band.vertex_count), bounds, band.colour, polygon_winding::preserve);
}

void draw_gouraud_polygon(framebuffer &target, std::span<projected_vertex const> const vertices, std::span<uint16_t const> const ramp, rectangle const bounds) {
  if(vertices.size() < 3) return;
  std::vector<uint8_t> colours;
  colours.reserve(vertices.size());
  for(auto const &vertex : vertices) {
    if(vertex.shade > 255) throw std::out_of_range{"Gouraud intensity exceeds native byte range"};
    colours.push_back(shade_colour(ramp, static_cast<uint8_t>(vertex.shade)));
  }
  for(size_t index{2}; index != vertices.size(); ++index) {
    std::array<vec2<int32_t>, 3> const points{vec2<int32_t>{vertices[0].position}, vec2<int32_t>{vertices[index - 1].position}, vec2<int32_t>{vertices[index].position}};
    draw_gouraud_triangle(target, points, {colours[0], colours[index - 1], colours[index]}, bounds);
  }
}

} // namespace earthsiege
