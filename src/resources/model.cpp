#include "model.h"
#include <limits>
#include <utility>

namespace earthsiege {
namespace {

struct object_chunk {
  model_kind kind{};
  bytes data;
};

auto chunk(binary_reader &file)->object_chunk {
  auto const kind{static_cast<model_kind>(file.integer<uint32_t>())};
  auto const size{file.integer<uint32_t>()};
  auto const data{file.take(size)};
  if((size & 1) && file.remaining()) file.take(1);
  return object_chunk{
    .kind{kind},
    .data{data},
  };
}

template<typename F>
auto counted(binary_reader &file, F read) {
  using T = decltype(read());
  auto const count{file.integer<uint16_t>()};
  std::vector<T> result;
  result.reserve(count);
  for(unsigned int index{}; index < count; ++index) result.push_back(read());
  return result;
}

auto material(binary_reader &file)->material_slot {
  return material_slot{
    .value{file.integer<int16_t>()},
    .flags{file.integer<uint16_t>()},
  };
}

auto polygon(binary_reader &parent)->model_polygon {
  auto const object{chunk(parent)};
  binary_reader file{object.data, "model polygon"};
  model_polygon result{
    .kind{object.kind},
    .normal{file.integer<uint16_t>()},
    .centre{file.integer<uint16_t>()},
    .vertex_count{file.integer<uint16_t>()},
    .vertex_list{file.integer<uint16_t>()},
    .material{},
    .normal_list{},
  };
  switch(object.kind) {
    case model_kind::helper_polygon: break;
    case model_kind::solid_polygon:
    case model_kind::shaded_polygon:
    case model_kind::gouraud_polygon:
    case model_kind::textured_polygon:
    case model_kind::aliased_polygon:
    case model_kind::aliased_shaded_polygon:
    case model_kind::aliased_gouraud_polygon: {
      auto const slot{file.integer<uint16_t>()};
      if(slot % 4) file.fail("face material does not begin at its front-fill slot");
      result.material = static_cast<uint16_t>(slot / 4);
      if(object.kind == model_kind::gouraud_polygon || object.kind == model_kind::aliased_gouraud_polygon) result.normal_list = file.integer<uint16_t>();
      break;
    }
    default: file.fail("non-polygon object inside mesh surface array");
  }
  file.require_end();
  return result;
}

auto sequence(binary_reader &parent)->animation_sequence {
  auto const object{chunk(parent)};
  if(object.kind != model_kind::sequence && object.kind != model_kind::cyclic_sequence) parent.fail("non-sequence inside animation list");
  binary_reader file{object.data, "animation sequence"};
  animation_sequence result;
  result.cyclic = object.kind == model_kind::cyclic_sequence;
  result.tick = file.integer<int16_t>();
  result.priority = file.integer<int16_t>();
  result.ground_movement = file.integer<int16_t>();
  result.frames = counted(file, [&file]{
    return animation_frame{
      .duration{file.integer<uint16_t>()},
      .transition_count{file.integer<uint16_t>()},
      .first_transition{file.integer<uint16_t>()},
    };
  });
  result.parts = counted(file, [&file]{ return file.integer<uint16_t>(); });
  auto const indices{result.parts.size() * result.frames.size()};
  if(indices > file.remaining() / 2) file.fail("animation pose table exceeds sequence extent");
  result.pose_indices.reserve(indices);
  for(size_t index{}; index < indices; ++index) result.pose_indices.push_back(file.integer<uint16_t>());
  file.require_end();
  return result;
}

auto animation(binary_reader &parent)->animation_list {
  auto const object{chunk(parent)};
  if(object.kind != model_kind::animation_list) parent.fail("expected animation list");
  binary_reader file{object.data, "animation list"};
  animation_list result;
  result.sequences = counted(file, [&file]{ return sequence(file); });
  result.transitions = counted(file, [&file]{
    return animation_transition{
      .tick{file.integer<uint16_t>()},
      .destination_sequence{file.integer<uint16_t>()},
      .destination_frame{file.integer<uint16_t>()},
      .ground_movement{file.integer<uint16_t>()},
    };
  });
  result.poses = counted(file, [&file]{
    return animation_pose{
      .rotation{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()},
      .translation{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()},
    };
  });
  result.defaults = counted(file, [&file]{ return file.integer<uint16_t>(); });
  result.relations = counted(file, [&file]{
    return node_relation{
      .parent{file.integer<int16_t>()},
      .node{file.integer<int16_t>()},
    };
  });
  file.require_end();
  return result;
}

auto heightfield(binary_reader &file)->collision_heightfield {
  collision_heightfield result{
    .size{file.integer<int16_t>(), file.integer<int16_t>()},
    .origin{file.integer<int16_t>(), file.integer<int16_t>()},
    .cell_shift{file.integer<int16_t>()},
    .heights{},
    .cells{},
  };
  if(result.size.x < 0 || result.size.y < 0) file.fail("negative collision heightfield dimensions");
  for(auto &height : result.heights) height = file.integer<int32_t>();
  size_t const amount{static_cast<size_t>(result.size.x) * static_cast<size_t>(result.size.y)};
  auto const cells{file.take(amount)};
  result.cells.assign(cells.begin(), cells.end());
  return result;
}

auto part(binary_reader &parent, unsigned int depth)->model_part {
  unsigned int constexpr maximum_model_depth{128};
  if(depth > maximum_model_depth) parent.fail("excessive model hierarchy depth");
  auto const object{chunk(parent)};
  binary_reader file{object.data, "model part"};
  model_part result;
  result.kind = object.kind;
  if(object.kind == model_kind::null) {
    file.require_end();
    return result;
  }
  result.header = part_header{
    .transform{file.integer<int16_t>()},
    .id{file.integer<int16_t>()},
    .radius{file.integer<int16_t>()},
    .centre{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()},
  };
  auto const children{[&file, depth]{ return counted(file, [&file, depth]{ return part(file, depth + 1); }); }};
  switch(object.kind) {
    case model_kind::base_part: break;
    case model_kind::part_list:
      result.content = part_list{
        .children{children()},
      };
      break;
    case model_kind::shape:
    case model_kind::animated_shape:
    case model_kind::terrain_shape: {
      shape_part shape;
      shape.children = children();
      auto const transforms{file.integer<uint16_t>()};
      shape.initial_cell_frames = counted(file, [&file]{ return file.integer<int16_t>(); });
      shape.transforms.reserve(transforms);
      for(unsigned int index{}; index < transforms; ++index) {
        model_transform transform;
        for(auto &value : transform.basis.data) value = file.integer<int16_t>();
        transform.type = file.integer<uint8_t>();
        transform.flags = file.integer<uint8_t>();
        transform.translation = vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
        shape.transforms.push_back(std::move(transform));
      }
      if(object.kind == model_kind::animated_shape) shape.animation = animation(file);
      if(object.kind == model_kind::terrain_shape) shape.collision = heightfield(file);
      result.content = std::move(shape);
      break;
    }
    case model_kind::cell_animation: {
      cell_part cells;
      cells.children = children();
      cells.sequence = file.integer<int16_t>();
      result.content = std::move(cells);
      break;
    }
    case model_kind::detail: {
      detail_part detail;
      detail.children = children();
      detail.distances.reserve(detail.children.size());
      for(unsigned int index{}; index < detail.children.size(); ++index) detail.distances.push_back(file.integer<int16_t>());
      result.content = std::move(detail);
      break;
    }
    case model_kind::billboard:
      result.content = billboard_part{
        .bitmap{file.integer<uint16_t>()},
        .offset{file.integer<int8_t>(), file.integer<int8_t>()},
      };
      break;
    case model_kind::bsp_part: {
      bsp_part bsp;
      bsp.children = children();
      bsp.nodes = counted(file, [&file]{
        return part_bsp_node{
          .normal{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()},
          .constant{file.integer<int32_t>()},
          .front{file.integer<int16_t>()},
          .back{file.integer<int16_t>()},
          .transform{-1},
        };
      });
      for(auto &node : bsp.nodes) node.transform = file.integer<int16_t>();
      result.content = std::move(bsp);
      break;
    }
    case model_kind::group:
    case model_kind::bsp_group: {
      auto const indices{file.integer<uint16_t>()};
      auto const points{file.integer<uint16_t>()};
      auto const slots{file.integer<uint16_t>()};
      auto const polygons{file.integer<uint16_t>()};
      if(slots % 4) file.fail("mesh material table lacks complete front/back slot groups");
      model_mesh mesh;
      mesh.indices.reserve(indices);
      mesh.points.reserve(points);
      mesh.materials.reserve(slots / 4);
      mesh.polygons.reserve(polygons);
      for(unsigned int index{}; index < indices; ++index) mesh.indices.push_back(file.integer<uint16_t>());
      for(unsigned int index{}; index < points; ++index) mesh.points.push_back(vec3<int16_t>{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()});
      for(unsigned int index{}; index < slots / 4; ++index) {
        mesh.materials.push_back(face_material{
          .front_fill{material(file)},
          .front_line{material(file)},
          .back_fill{material(file)},
          .back_line{material(file)},
        });
      }
      for(unsigned int index{}; index < polygons; ++index) mesh.polygons.push_back(polygon(file));
      if(object.kind == model_kind::bsp_group) {
        mesh.bsp = counted(file, [&file]{
          return polygon_bsp_node{
            .constant{file.integer<int32_t>()},
            .polygon{file.integer<int16_t>()},
            .front{file.integer<int16_t>()},
            .back{file.integer<int16_t>()},
          };
        });
      }
      result.content = std::move(mesh);
      break;
    }
    default: file.fail("unknown model part tag");
  }
  file.require_end();
  return result;
}

} // namespace

auto read_model(bytes source)->model_resource {
  binary_reader file{source, "3Space model"};
  model_resource result;
  while(file.remaining()) result.roots.push_back(part(file, 0));
  return result;
}

} // namespace earthsiege
