#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "resources/model.h"
#include "resources/resource_store.h"

namespace {

enum class field : unsigned int {
  tags, base, children, mesh_counts, indices, points, materials, polygons, bsp,
  detail, billboards, cell, shape, anim_counts, seq_header, seq_frames, seq_parts,
  seq_indices, poses, transitions, defaults, relations, collision, count,
};

class fingerprint {
  std::array<std::vector<uint8_t>, std::to_underlying(field::count)> buckets;

  void byte(field target, uint8_t value) { buckets[std::to_underlying(target)].push_back(value); }
  void word(field target, int32_t value) {
    byte(target, static_cast<uint8_t>(value));
    byte(target, static_cast<uint8_t>(static_cast<uint32_t>(value) >> 8));
  }
  void dword(field target, int32_t value) {
    for(unsigned int index{}; index < 4; ++index) byte(target, static_cast<uint8_t>(static_cast<uint32_t>(value) >> (index * 8)));
  }
  template<typename R>
  void words(field target, R const &values) { for(auto const value : values) word(target, value); }
  template<typename T>
  void vector(field target, vec3<T> const &point) { word(target, point.x); word(target, point.y); word(target, point.z); }
  void tag(earthsiege::model_kind kind) { dword(field::tags, static_cast<int32_t>(std::to_underlying(kind))); }

  void animation(earthsiege::animation_list const &source) {
    tag(earthsiege::model_kind::animation_list);
    word(field::anim_counts, static_cast<int32_t>(source.sequences.size()));
    word(field::anim_counts, static_cast<int32_t>(source.transitions.size()));
    word(field::anim_counts, static_cast<int32_t>(source.poses.size()));
    word(field::anim_counts, static_cast<int32_t>(source.defaults.size()));
    word(field::anim_counts, static_cast<int32_t>(source.relations.size()));
    for(auto const &sequence : source.sequences) {
      tag(sequence.cyclic ? earthsiege::model_kind::cyclic_sequence : earthsiege::model_kind::sequence);
      word(field::seq_header, sequence.tick); word(field::seq_header, sequence.priority); word(field::seq_header, sequence.ground_movement);
      word(field::seq_header, static_cast<int32_t>(sequence.frames.size()));
      for(auto const &frame : sequence.frames) {
        word(field::seq_frames, frame.duration); word(field::seq_frames, frame.transition_count); word(field::seq_frames, frame.first_transition);
      }
      word(field::seq_parts, static_cast<int32_t>(sequence.parts.size()));
      words(field::seq_parts, sequence.parts);
      words(field::seq_indices, sequence.pose_indices);
    }
    for(auto const &pose : source.poses) { vector(field::poses, pose.rotation); vector(field::poses, pose.translation); }
    for(auto const &transition : source.transitions) {
      word(field::transitions, transition.tick); word(field::transitions, transition.destination_sequence);
      word(field::transitions, transition.destination_frame); word(field::transitions, transition.ground_movement);
    }
    words(field::defaults, source.defaults);
    for(auto const &relation : source.relations) { word(field::relations, relation.parent); word(field::relations, relation.node); }
  }

  void material(earthsiege::material_slot const &slot) { word(field::materials, slot.value); word(field::materials, slot.flags); }

public:
  void visit(earthsiege::model_part const &part) {
    tag(part.kind);
    if(part.kind != earthsiege::model_kind::null) {
      word(field::base, part.header.transform); word(field::base, part.header.id); word(field::base, part.header.radius); vector(field::base, part.header.centre);
    }
    std::visit([this, &part](auto const &content){
      using T = std::remove_cvref_t<decltype(content)>;
      if constexpr(requires { content.children; }) {
        word(field::children, static_cast<int32_t>(content.children.size()));
        for(auto const &child : content.children) visit(child);
      }
      if constexpr(std::is_same_v<T, earthsiege::shape_part>) {
        word(field::shape, static_cast<int32_t>(content.transforms.size()));
        word(field::shape, static_cast<int32_t>(content.initial_cell_frames.size()));
        words(field::shape, content.initial_cell_frames);
        if(content.animation) animation(*content.animation);
        if(content.collision) {
          auto const &heightfield{*content.collision};
          word(field::collision, heightfield.size.x); word(field::collision, heightfield.size.y);
          word(field::collision, heightfield.origin.x); word(field::collision, heightfield.origin.y); word(field::collision, heightfield.cell_shift);
          for(auto const height : heightfield.heights) dword(field::collision, height);
          for(auto const code : heightfield.cells) byte(field::collision, code);
        }
      } else if constexpr(std::is_same_v<T, earthsiege::model_mesh>) {
        word(field::mesh_counts, static_cast<int32_t>(content.indices.size())); word(field::mesh_counts, static_cast<int32_t>(content.points.size()));
        word(field::mesh_counts, static_cast<int32_t>(content.materials.size() * 4)); word(field::mesh_counts, static_cast<int32_t>(content.polygons.size()));
        words(field::indices, content.indices);
        for(auto const &point : content.points) vector(field::points, point);
        for(auto const &surface : content.materials) { material(surface.front_fill); material(surface.front_line); material(surface.back_fill); material(surface.back_line); }
        for(auto const &polygon : content.polygons) {
          tag(polygon.kind);
          word(field::polygons, polygon.normal); word(field::polygons, polygon.centre); word(field::polygons, polygon.vertex_count); word(field::polygons, polygon.vertex_list);
          if(polygon.material) word(field::polygons, *polygon.material * 4);
          if(polygon.normal_list) word(field::polygons, *polygon.normal_list);
        }
        if(part.kind == earthsiege::model_kind::bsp_group) {
          word(field::bsp, std::to_underlying(part.kind) & 65535); word(field::bsp, static_cast<int32_t>(content.bsp.size()));
          for(auto const &node : content.bsp) {
            dword(field::bsp, node.constant); word(field::bsp, node.polygon); word(field::bsp, node.front); word(field::bsp, node.back);
          }
        }
      } else if constexpr(std::is_same_v<T, earthsiege::bsp_part>) {
        word(field::bsp, std::to_underlying(part.kind) & 65535); word(field::bsp, static_cast<int32_t>(content.nodes.size()));
        for(auto const &node : content.nodes) {
          vector(field::bsp, node.normal); dword(field::bsp, node.constant); word(field::bsp, node.front); word(field::bsp, node.back);
        }
        for(auto const &node : content.nodes) word(field::bsp, node.transform);
      } else if constexpr(std::is_same_v<T, earthsiege::detail_part>) words(field::detail, content.distances);
      else if constexpr(std::is_same_v<T, earthsiege::cell_part>) word(field::cell, content.sequence);
      else if constexpr(std::is_same_v<T, earthsiege::billboard_part>) {
        word(field::billboards, content.bitmap); byte(field::billboards, static_cast<uint8_t>(content.offset.x)); byte(field::billboards, static_cast<uint8_t>(content.offset.y));
      }
    }, part.content);
  }

  auto checksums() const->std::array<uint32_t, std::to_underlying(field::count)> {
    std::array<uint32_t, std::to_underlying(field::count)> result{};
    for(unsigned int index{}; index < result.size(); ++index) result[index] = earthsiege::crc32(buckets[index]);
    return result;
  }
  auto objects() const->size_t { return buckets[std::to_underlying(field::tags)].size() / 4; }
};

} // namespace

auto main(int argc, char *argv[])->int try {
  if(argc != 3) throw std::runtime_error{"expected assets and native model fingerprints"};
  earthsiege::resource_store const resources{argv[1]};
  std::ifstream fixture{argv[2]};
  std::string volume;
  std::string name;
  unsigned int roots{};
  unsigned int objects{};
  unsigned int files{};
  size_t total{};
  while(fixture >> volume >> name >> roots >> objects) {
    std::array<uint32_t, std::to_underlying(field::count)> expected{};
    for(auto &value : expected) fixture >> std::hex >> value >> std::dec;
    auto const model{earthsiege::read_model(resources.read(volume, name))};
    fingerprint actual;
    for(auto const &root : model.roots) actual.visit(root);
    if(model.roots.size() != roots || actual.objects() != objects) throw std::runtime_error{"model hierarchy differs from original: " + name};
    auto const checksums{actual.checksums()};
    for(unsigned int index{}; index < expected.size(); ++index) {
      if(checksums[index] != expected[index]) throw std::runtime_error{"model field " + std::to_string(index) + " differs from original: " + name};
    }
    total += actual.objects();
    ++files;
  }
  if(files != 44 || total != 47575) throw std::runtime_error{"incomplete original model fixtures"};
  std::cout << files << " model files and " << total << " original object fingerprints agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
