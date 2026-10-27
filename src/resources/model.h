#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include "vectorstorm/matrix/matrix3.h"
#include "vectorstorm/vector/vector2.h"
#include "vectorstorm/vector/vector3.h"
#include "resources/binary_reader.h"

namespace earthsiege {

inline int16_t constexpr model_matrix_one{1 << 14};
inline int16_t constexpr model_normal_one{1 << 11};

enum class model_kind : uint32_t {
  helper_polygon = 0x140001,
  solid_polygon = 0x140002,
  shaded_polygon = 0x140003,
  base_part = 0x140005,
  part_list = 0x140007,
  shape = 0x140008,
  gouraud_polygon = 0x140009,
  bsp_group = 0x14000a,
  cell_animation = 0x14000b,
  detail = 0x14000c,
  null = 0x14000e,
  textured_polygon = 0x14000f,
  aliased_polygon = 0x140010,
  aliased_shaded_polygon = 0x140011,
  aliased_gouraud_polygon = 0x140012,
  billboard = 0x140013,
  group = 0x140014,
  bsp_part = 0x140015,
  sequence = 0x1e0001,
  animation_list = 0x1e0002,
  animated_shape = 0x1e0003,
  cyclic_sequence = 0x1e0004,
  terrain_shape = 0x2bc0001,
};

struct part_header {
  int16_t transform{-1};
  int16_t id{};
  int16_t radius{};
  vec3<int16_t> centre;
};

struct material_slot {
  int16_t value{};
  uint16_t flags{};
};

struct face_material {
  material_slot front_fill;
  material_slot front_line;
  material_slot back_fill;
  material_slot back_line;
};

struct model_polygon {
  model_kind kind{};
  uint16_t normal{};
  uint16_t centre{};
  uint16_t vertex_count{};
  uint16_t vertex_list{};
  std::optional<uint16_t> material;
  std::optional<uint16_t> normal_list;
};

struct polygon_bsp_node {
  int32_t constant{};
  int16_t polygon{};
  int16_t front{};
  int16_t back{};
};

struct model_mesh {
  std::vector<uint16_t> indices;
  std::vector<vec3<int16_t>> points;
  std::vector<face_material> materials;
  std::vector<model_polygon> polygons;
  std::vector<polygon_bsp_node> bsp;
};

struct animation_frame {
  uint16_t duration{};
  uint16_t transition_count{};
  uint16_t first_transition{};
};

struct animation_sequence {
  bool cyclic{};
  int16_t tick{};
  int16_t priority{};
  int16_t ground_movement{};
  std::vector<animation_frame> frames;
  std::vector<uint16_t> parts;
  std::vector<uint16_t> pose_indices;
};

struct animation_transition {
  uint16_t tick{};
  uint16_t destination_sequence{};
  uint16_t destination_frame{};
  uint16_t ground_movement{};
};

struct animation_pose {
  vec3<uint16_t> rotation;
  vec3<int16_t> translation;
};

struct node_relation {
  int16_t parent{};
  int16_t node{};
};

struct animation_list {
  std::vector<animation_sequence> sequences;
  std::vector<animation_transition> transitions;
  std::vector<animation_pose> poses;
  std::vector<uint16_t> defaults;
  std::vector<node_relation> relations;
};

struct model_transform {
  mat3<int16_t> basis{
    model_matrix_one, int16_t{}, int16_t{},
    int16_t{}, model_matrix_one, int16_t{},
    int16_t{}, int16_t{}, model_matrix_one,
  };
  uint8_t type{};
  uint8_t flags{};
  vec3<int32_t> translation;
};

struct collision_heightfield {
  vec2<int16_t> size;
  vec2<int16_t> origin;
  int16_t cell_shift{};
  std::array<int32_t, 256> heights{};
  std::vector<uint8_t> cells;
};

struct model_part;

struct part_list {
  std::vector<model_part> children;
};

struct shape_part {
  std::vector<model_part> children;
  std::vector<int16_t> cell_frame_counts;
  std::vector<model_transform> transforms;
  std::optional<animation_list> animation;
  std::optional<collision_heightfield> collision;
};

struct part_bsp_node {
  vec3<int16_t> normal;
  int32_t constant{};
  int16_t front{};
  int16_t back{};
  int16_t transform{-1};
};

struct bsp_part {
  std::vector<model_part> children;
  std::vector<part_bsp_node> nodes;
};

struct detail_part {
  std::vector<model_part> children;
  std::vector<int16_t> distances;
};

struct cell_part {
  std::vector<model_part> children;
  int16_t sequence{};
};

struct billboard_part {
  uint16_t bitmap{};
  vec2<int8_t> offset;
};

struct empty_part {};
using model_part_content = std::variant<empty_part, part_list, shape_part, model_mesh, bsp_part, detail_part, cell_part, billboard_part>;

struct model_part {
  model_kind kind{model_kind::null};
  part_header header;
  model_part_content content;

  model_part() = default;
  model_part(model_part const &) = delete;
  auto operator=(model_part const &)->model_part & = delete;
  model_part(model_part &&) noexcept = default;
  auto operator=(model_part &&) noexcept->model_part & = default;
};

struct model_resource {
  std::vector<model_part> roots;
};

auto read_model(bytes source)->model_resource;

} // namespace earthsiege
