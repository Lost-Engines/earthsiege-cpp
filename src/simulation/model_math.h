#pragma once
#include <cstdint>
#include "resources/model.h"

namespace earthsiege {

inline unsigned int constexpr angle_turn{1U << 16};
inline unsigned int constexpr angle_quarter_turn{angle_turn / 4};
inline unsigned int constexpr matrix_fraction_bits{14};
inline int32_t constexpr matrix_rounding{1 << (matrix_fraction_bits - 1)};

// Original angles wrap at 65536. Table sampling rounds the low four bits,
// including negative half steps, exactly as the original matrix builder does.
auto model_cosine(uint16_t angle)->int16_t;
uint16_t model_arcsine(int16_t sine);
vec3<uint16_t> transform_angles(model_transform const &transform);
uint16_t direction_angle(int32_t x, int32_t y);
auto rotation_transform(vec3<uint16_t> const &angles)->model_transform;
auto transform_point(vec3<int16_t> const &point, model_transform const &transform)->vec3<int32_t>;
auto rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t>;
auto inverse_rotate_vector(vec3<int32_t> const &vector, model_transform const &transform)->vec3<int32_t>;
model_transform inverse_transform(model_transform const &source);
auto compose_transforms(model_transform const &local, model_transform const &parent)->model_transform;
auto interpolate_pose(animation_pose const &from, animation_pose const &to, int16_t fraction)->animation_pose;
auto inverse_plane_distance(vec3<int32_t> const &eye, model_transform const &transform, vec3<int16_t> const &normal)->int32_t;

} // namespace earthsiege
