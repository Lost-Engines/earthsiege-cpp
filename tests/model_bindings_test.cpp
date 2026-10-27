#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "resources/binary_reader.h"
#include "resources/model_bindings.h"
#include "resources/resource_store.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESBIND01") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};
  unsigned int total{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const slash{name.find('/')};
    auto model{earthsiege::read_model(resources.read(name.substr(0, slash), name.substr(slash + 1)))};
    auto const length{file.integer<uint16_t>()};
    auto const ids{file.take(length)};
    auto const values{file.integer<uint32_t>()};
    earthsiege::bind_herc_detail_nodes(model, ids);
    unsigned int checked{0};
    auto const check{[&](int16_t const actual){
      if(actual != file.integer<int16_t>()) throw std::runtime_error{"detail binding differs in " + name + " at " + std::to_string(checked)};
      ++checked;
    }};
    auto const visit{[&](this auto const &self, earthsiege::model_part const &part)->void {
      check(part.header.transform);
      std::visit([&](auto const &content){
        using type = std::decay_t<decltype(content)>;
        if constexpr(std::is_same_v<type, earthsiege::bsp_part>) for(auto const &node : content.nodes) check(node.transform);
        if constexpr(requires { content.children; }) for(auto const &child : content.children) self(child);
      }, part.content);
    }};
    for(auto const &part : model.roots) visit(part);
    if(checked != values) file.fail("binding node count differs");
    total += checked;
  }
  file.require_end();
  if(count != 16) file.fail("incomplete binding fixtures");
  std::cout << count << " original HERC detail bindings match " << total << " part/node transforms" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
