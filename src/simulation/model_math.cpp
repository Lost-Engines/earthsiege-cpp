#include "model_math.h"
#include <bit>
#include <cstdlib>
#include "arcsine_table.h"
#include "arctangent_table.h"
#include "cosine_table.h"
#include "fixed_math.h"

namespace earthsiege {
namespace {

int32_t rounded_product(int64_t const value) {
  return wrap_long((value + matrix_rounding) >> matrix_fraction_bits);
}
int32_t cosine(int const value) {
  return model_cosine(static_cast<uint16_t>(value));
}

auto rotate(vec3<int32_t> const &point, model_transform const &transform, bool const inverse)->vec3<int32_t> {
  if(transform.type == 0) return vec3<int32_t>{point};
  auto const &m{transform.basis};
  auto component{[&](unsigned int const row)->int32_t {
    int64_t sum{0};
    for(unsigned int column{0}; column < 3; ++column) {
      sum += static_cast<int64_t>(point[column]) * (inverse ? m.at(row, column) : m.at(column, row));
    }
    return rounded_product(sum);
  }};
  if(transform.type == 1) {
    return vec3<int32_t>{component(0), component(1), point.z};
  }
  return vec3<int32_t>{component(0), component(1), component(2)};
}

} // anonymous namespace

auto model_cosine(uint16_t const angle)->int16_t {
  int const signed_angle{std::bit_cast<int16_t>(angle)};
  int const sample{(signed_angle >> 4) + ((signed_angle >> 3) & 1)};
  unsigned int const index{static_cast<unsigned int>(sample < 0 ? -sample : sample)};
  return index < 1024 ? cosine_table[index] : static_cast<int16_t>(-cosine_table[2048 - index]);
}

uint16_t model_arcsine(int16_t const sine) {
  auto const magnitude{std::abs(static_cast<int>(sine))};
  auto const index{static_cast<size_t>((magnitude + 2) >> 2)};
  auto const angle{arcsine_table.at(index)};
  return static_cast<uint16_t>(sine < 0 ? -angle : angle);
}

vec3<uint16_t> transform_angles(model_transform const &transform) {
  auto const &m{transform.basis.data};
  auto const pitch{model_arcsine(m[5])};
  if(std::abs(static_cast<int>(wrap_word(pitch))) == angle_quarter_turn) return vec3<uint16_t>{pitch, 0, direction_angle(m[0], m[1])};
  return vec3<uint16_t>{pitch, direction_angle(m[8], -m[2]), direction_angle(m[4], -m[3])};
}

uint16_t direction_angle(int32_t const x, int32_t const y) {
  /// Original octant lookup, including rounding without carry into the high word
  auto const ax{static_cast<uint64_t>(std::abs(static_cast<int64_t>(x)))};
  auto const ay{static_cast<uint64_t>(std::abs(static_cast<int64_t>(y)))};
  auto const ratio_angle{[](uint64_t const small, uint64_t const large) {
    if(small == 0) return 0;
    auto const product{small * 512};
    auto const rounded{(product & ~uint64_t{0xffffffff}) | static_cast<uint32_t>(product + (large >> 1))};
    auto const index{static_cast<size_t>(rounded / large)};
    return static_cast<int>(index * 4 + arctangent_table.at(index));
  }};
  int angle{ay <= ax ? ratio_angle(ay, ax) : 4096 - ratio_angle(ax, ay)};
  if(x < 0) angle = 8192 - angle;
  if(y < 0) angle = 16384 - angle;
  return static_cast<uint16_t>(angle * 4);
}

auto rotation_transform(vec3<uint16_t> const &angles)->model_transform {
  model_transform result;
  if(angles.x == 0 && angles.y == 0) {
    if(angles.z == 0) return result;
    int16_t const c{model_cosine(angles.z)};
    int16_t const s{model_cosine(static_cast<uint16_t>(angles.z - angle_quarter_turn))};
    result.basis = mat3<int16_t>{c, s, int16_t{}, static_cast<int16_t>(-s), c, int16_t{}, int16_t{}, int16_t{}, model_matrix_one};
    result.type = 1;
    return result;
  }
  int const a{angles.x};
  int const b{angles.y};
  int const c{angles.z};
  int const quarter{static_cast<int>(angle_quarter_turn)};
  int const s{cosine(a - quarter)};
  int const plus_cos{(cosine(b - c) + cosine(b + c)) >> 1};
  int const minus_cos{(cosine(b - c) - cosine(b + c)) >> 1};
  int const plus_sin{(cosine(b + c - quarter) + cosine(c - b - quarter)) >> 1};
  int const minus_sin{(cosine(b + c - quarter) - cosine(c - b - quarter)) >> 1};
  // the original builder uses sum/difference identities, with rounding at each
  // intermediate product; multiplying three independent Euler matrices changes
  // those rounding points and gives different articulated vertex positions
  result.basis = mat3<int16_t>{
    wrap_word(plus_cos - rounded_product(static_cast<int64_t>(s) * minus_cos)),
    wrap_word(plus_sin + rounded_product(static_cast<int64_t>(s) * minus_sin)),
    wrap_word(-(cosine(b - a - quarter) + cosine(b + a - quarter)) >> 1),
    wrap_word(-((cosine(c - a - quarter) + cosine(c + a - quarter)) >> 1)),
    wrap_word((cosine(a - c) + cosine(c + a)) >> 1),
    wrap_word(s),
    wrap_word(minus_sin + rounded_product(static_cast<int64_t>(s) * plus_sin)),
    wrap_word(minus_cos - rounded_product(static_cast<int64_t>(s) * plus_cos)),
    wrap_word((cosine(a - b) + cosine(b + a)) >> 1),
  };
  result.type = angles.x ? 3 : 2;
  return result;
}

auto transform_point(vec3<int16_t> const &point, model_transform const &transform)->vec3<int32_t> {
  auto const &m{transform.basis};
  auto component{[&](unsigned int const row)->int32_t {
    int32_t value{point[row]};
    if(transform.type && (transform.type != 1 || row != 2)) {
      int64_t sum{matrix_rounding};
      for(unsigned int column{0}; column < 3; ++column) sum += static_cast<int64_t>(point[column]) * m.at(column, row);
      // the short-point path accumulates in 32 bits before its arithmetic shift
      value = wrap_long(sum) >> matrix_fraction_bits;
    }
    return wrap_long(static_cast<int64_t>(value) + transform.translation[row]);
  }};
  return vec3<int32_t>{component(0), component(1), component(2)};
}

auto rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t> {
  return rotate(vector, transform, false);
}

auto inverse_rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t> {
  return rotate(vector, transform, true);
}

model_transform inverse_transform(model_transform const &source) {
  auto inverse{source};
  for(unsigned int column{0}; column != 3; ++column) {
    for(unsigned int row{0}; row != 3; ++row) inverse.basis.at(column, row) = source.basis.at(row, column);
  }
  vec3<int32_t> const translation{wrap_long(-static_cast<int64_t>(source.translation.x)), wrap_long(-static_cast<int64_t>(source.translation.y)), wrap_long(-static_cast<int64_t>(source.translation.z))};
  inverse.translation = rotate_vector(translation, inverse);
  return inverse;
}

auto compose_transforms(model_transform const &local, model_transform const &parent)->model_transform {
  model_transform result;
  if(local.type == 0) result = model_transform{parent};
  else if(parent.type == 0) result = model_transform{local};
  else {
    for(unsigned int column{0}; column < 3; ++column) {
      for(unsigned int row{0}; row < 3; ++row) {
        int64_t sum{0};
        for(unsigned int k{0}; k < 3; ++k) sum += static_cast<int64_t>(parent.basis.at(k, row)) * local.basis.at(column, k);
        result.basis.at(column, row) = wrap_word(rounded_product(sum));
      }
    }
    result.type = local.type == 1 && parent.type == 1 ? 1 : 2;
  }
  auto const translation{rotate_vector(local.translation, parent)};
  for(unsigned int axis{0}; axis < 3; ++axis) result.translation[axis] = wrap_long(static_cast<int64_t>(translation[axis]) + parent.translation[axis]);
  return result;
}

auto interpolate_pose(animation_pose const &from, animation_pose const &to, int16_t const fraction)->animation_pose {
  animation_pose result;
  unsigned int constexpr interpolation_bits{10};
  for(unsigned int axis{0}; axis < 3; ++axis) {
    // a signed 16-bit difference chooses the shortest angular arc; exactly a
    // half turn goes in the negative direction, matching the original tie rule
    int const turn{wrap_word(static_cast<int>(to.rotation[axis]) - from.rotation[axis])};
    result.rotation[axis] = static_cast<uint16_t>(from.rotation[axis] + ((turn * fraction) >> interpolation_bits));
    // translation differences also wrap to a signed word before interpolation
    int const movement{wrap_word(static_cast<int>(to.translation[axis]) - from.translation[axis])};
    result.translation[axis] = wrap_word(from.translation[axis] + ((movement * fraction) >> interpolation_bits));
  }
  return result;
}

auto inverse_plane_distance(vec3<int32_t> const &eye, model_transform const &transform, vec3<int16_t> const &normal)->int32_t {
  vec3<int32_t> const relative{
    wrap_long(static_cast<int64_t>(eye.x) - transform.translation.x),
    wrap_long(static_cast<int64_t>(eye.y) - transform.translation.y),
    wrap_long(static_cast<int64_t>(eye.z) - transform.translation.z),
  };
  // BSP plane tests always evaluate the complete inverse matrix, regardless of
  // the identity/planar fast-path tag used for ordinary vertex transforms
  auto matrix{model_transform{transform}};
  matrix.type = 2;
  auto const local{inverse_rotate_vector(relative, matrix)};
  return wrap_long(static_cast<int64_t>(local.x) * normal.x + static_cast<int64_t>(local.y) * normal.y + static_cast<int64_t>(local.z) * normal.z);
}

uint16_t bearing_angle(vec3<int32_t> const &first, vec3<int32_t> const &second) {
  auto x{wrap_long(static_cast<int64_t>(second.x) - first.x)};
  auto const y{wrap_long(static_cast<int64_t>(second.y) - first.y)};
  if(x == 0 && y == 0) x = 1;
  return static_cast<uint16_t>(direction_angle(x, y) - angle_quarter_turn);
}

} // namespace earthsiege
