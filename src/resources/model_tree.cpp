#include "model_tree.h"
#include <type_traits>
#include <utility>

namespace earthsiege {

std::span<model_part> model_children(model_part &part) {
  return std::visit([](auto &content)->std::span<model_part> {
    if constexpr(requires { content.children; }) return content.children;
    else return {};
  }, part.content);
}

std::span<model_part const> model_children(model_part const &part) {
  return std::visit([](auto const &content)->std::span<model_part const> {
    if constexpr(requires { content.children; }) return content.children;
    else return {};
  }, part.content);
}

model_part const *find_model_part(model_part const &root, int16_t const id) {
  if(root.kind != model_kind::null && root.header.id == id) return &root;
  for(auto const &child : model_children(root)) if(auto const *found{find_model_part(child, id)}) return found;
  return nullptr;
}

model_part clone_model_part(model_part const &source) {
  model_part result;
  result.kind = source.kind;
  result.header = source.header;
  std::visit([&](auto const &content) {
    using type = std::decay_t<decltype(content)>;
    if constexpr(requires { content.children; }) {
      type copy;
      if constexpr(std::is_same_v<type, shape_part>) {
        copy.cell_frame_counts = content.cell_frame_counts;
        copy.transforms = content.transforms;
        copy.animation = content.animation;
        copy.collision = content.collision;
      } else if constexpr(std::is_same_v<type, bsp_part>) copy.nodes = content.nodes;
      else if constexpr(std::is_same_v<type, detail_part>) copy.distances = content.distances;
      else if constexpr(std::is_same_v<type, cell_part>) copy.sequence = content.sequence;
      copy.children.reserve(content.children.size());
      for(auto const &child : content.children) copy.children.push_back(clone_model_part(child));
      result.content = std::move(copy);
    } else result.content = content;
  }, source.content);
  return result;
}

} // namespace earthsiege
