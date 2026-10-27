#include "model_math.h"
#include <bit>
#include "cosine_table.h"

namespace earthsiege {
namespace {

auto word(int value)->int16_t { return std::bit_cast<int16_t>(static_cast<uint16_t>(value)); }
auto signed_long(int64_t value)->int32_t { return std::bit_cast<int32_t>(static_cast<uint32_t>(value)); }
auto rounded_product(int64_t value)->int32_t { return signed_long((value + matrix_rounding) >> matrix_fraction_bits); }
auto cosine(int value)->int32_t { return model_cosine(static_cast<uint16_t>(value)); }

auto rotate(vec3<int32_t> const &point, model_transform const &transform, bool inverse)->vec3<int32_t> {
  if(transform.type == 0) return vec3<int32_t>{point};
  auto const &m{transform.basis};
  auto component{[&](unsigned int row)->int32_t {
    int64_t sum{};
    for(unsigned int column{}; column < 3; ++column) {
      sum += static_cast<int64_t>(point[column]) * (inverse ? m.at(row, column) : m.at(column, row));
    }
    return rounded_product(sum);
  }};
  if(transform.type == 1) {
    return vec3<int32_t>{component(0), component(1), point.z};
  }
  return vec3<int32_t>{component(0), component(1), component(2)};
}

} // namespace

auto model_cosine(uint16_t angle)->int16_t {
  int const signed_angle{std::bit_cast<int16_t>(angle)};
  int const sample{(signed_angle >> 4) + ((signed_angle >> 3) & 1)};
  unsigned int const index{static_cast<unsigned int>(sample < 0 ? -sample : sample)};
  return index < 1024 ? cosine_table[index] : static_cast<int16_t>(-cosine_table[2048 - index]);
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
  // The original builder uses sum/difference identities, with rounding at each
  // intermediate product. Multiplying three independent Euler matrices changes
  // those rounding points and gives different articulated vertex positions.
  result.basis = mat3<int16_t>{
    word(plus_cos - rounded_product(static_cast<int64_t>(s) * minus_cos)),
    word(plus_sin + rounded_product(static_cast<int64_t>(s) * minus_sin)),
    word(-(cosine(b - a - quarter) + cosine(b + a - quarter)) >> 1),
    word(-((cosine(c - a - quarter) + cosine(c + a - quarter)) >> 1)),
    word((cosine(a - c) + cosine(c + a)) >> 1),
    word(s),
    word(minus_sin + rounded_product(static_cast<int64_t>(s) * plus_sin)),
    word(minus_cos - rounded_product(static_cast<int64_t>(s) * plus_cos)),
    word((cosine(a - b) + cosine(b + a)) >> 1),
  };
  result.type = angles.x ? 3 : 2;
  return result;
}

auto transform_point(vec3<int16_t> const &point, model_transform const &transform)->vec3<int32_t> {
  auto const &m{transform.basis};
  auto component{[&](unsigned int row)->int32_t {
    int32_t value{point[row]};
    if(transform.type && (transform.type != 1 || row != 2)) {
      int64_t sum{matrix_rounding};
      for(unsigned int column{}; column < 3; ++column) sum += static_cast<int64_t>(point[column]) * m.at(column, row);
      // The short-point path accumulates in 32 bits before its arithmetic shift.
      value = signed_long(sum) >> matrix_fraction_bits;
    }
    return signed_long(static_cast<int64_t>(value) + transform.translation[row]);
  }};
  return vec3<int32_t>{component(0), component(1), component(2)};
}

auto rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t> {
  return rotate(vector, transform, false);
}

auto inverse_rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t> {
  return rotate(vector, transform, true);
}

auto inverse_plane_distance(vec3<int32_t> const &eye, model_transform const &transform, vec3<int16_t> const &normal)->int32_t {
  vec3<int32_t> const relative{
    signed_long(static_cast<int64_t>(eye.x) - transform.translation.x),
    signed_long(static_cast<int64_t>(eye.y) - transform.translation.y),
    signed_long(static_cast<int64_t>(eye.z) - transform.translation.z),
  };
  // BSP plane tests always evaluate the complete inverse matrix, regardless of
  // the identity/planar fast-path tag used for ordinary vertex transforms.
  auto matrix{model_transform{transform}};
  matrix.type = 2;
  auto const local{inverse_rotate_vector(relative, matrix)};
  return signed_long(static_cast<int64_t>(local.x) * normal.x + static_cast<int64_t>(local.y) * normal.y + static_cast<int64_t>(local.z) * normal.z);
}

} // namespace earthsiege
