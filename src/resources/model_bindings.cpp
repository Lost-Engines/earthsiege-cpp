#include "model_bindings.h"
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace earthsiege {
namespace {

template<typename F>
void children(model_part &part, F const &visit) {
  std::visit([&](auto &content){
    using type = std::decay_t<decltype(content)>;
    if constexpr(std::is_same_v<type, shape_part> || std::is_same_v<type, part_list> || std::is_same_v<type, bsp_part> || std::is_same_v<type, detail_part> || std::is_same_v<type, cell_part>) {
      for(auto &child : content.children) visit(child);
    }
  }, part.content);
}

model_part *find_part(model_part &part, uint8_t const id) {
  if(part.kind != model_kind::null && part.header.id == id) return &part;
  model_part *found{nullptr};
  children(part, [&](auto &child){ if(!found) found = find_part(child, id); });
  return found;
}

void bind_part(model_part &part, std::span<int16_t const> const indices, size_t const pose_count) {
  if((part.kind == model_kind::group || part.kind == model_kind::cell_animation) && part.header.transform >= 0) {
    auto const index{static_cast<size_t>(part.header.transform)};
    if(index >= indices.size()) throw std::out_of_range{"detail transform exceeds native binding row"};
    part.header.transform = indices[index];
  }
  if(auto *tree{std::get_if<bsp_part>(&part.content)}) {
    for(auto &node : tree->nodes) {
      if(node.transform > 0 && static_cast<size_t>(node.transform) < pose_count && indices[static_cast<size_t>(node.transform)] >= 0) node.transform = indices[static_cast<size_t>(node.transform)];
    }
  }
  children(part, [&](auto &child){ bind_part(child, indices, pose_count); });
}

} // namespace

void bind_herc_detail_nodes(model_resource &model, std::span<uint8_t const> const part_ids) {
  if(model.roots.size() < 2) return;
  auto const &full{std::get<shape_part>(model.roots.front().content)};
  if(!full.animation) throw std::invalid_argument{"HERC detail binding requires a full-detail animation"};
  size_t constexpr binding_row_size{50};
  auto const pose_count{full.animation->defaults.size()};
  if(pose_count > binding_row_size) throw std::out_of_range{"HERC pose count exceeds native binding row"};
  std::vector<int16_t> bindings(model.roots.size() * binding_row_size, -1);
  for(size_t root{1}; root != model.roots.size(); ++root) {
    for(auto const id : part_ids) {
      if(id >= 128) break;
      auto const *base{find_part(model.roots.front(), id)};
      if(!base) throw std::runtime_error{"HERC profile references an absent full-detail part"};
      auto const *part{find_part(model.roots[root], id)};
      if(!part) continue;
      auto const index{static_cast<int64_t>(root * binding_row_size) + part->header.transform};
      if(index < 0 || static_cast<uint64_t>(index) >= bindings.size()) throw std::out_of_range{"HERC transform exceeds native binding scratch"};
      // Static (-1) parts touch the preceding row's last entry in the original.
      bindings[static_cast<size_t>(index)] = base->header.transform;
    }
  }
  for(size_t root{1}; root != model.roots.size(); ++root) bind_part(model.roots[root], std::span{bindings}.subspan(root * binding_row_size, binding_row_size), pose_count);
}

} // namespace earthsiege
