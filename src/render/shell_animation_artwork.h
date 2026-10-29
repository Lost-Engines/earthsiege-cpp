#pragma once
#include "render/framebuffer.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"
#include "shell/shell_animation.h"
#include <array>
namespace earthsiege {
/// GO brush banks and retained background updates, separate from scheduling.
class shell_animation_artwork {
 resource_store const &resources;
 std::string directory;
 std::array<std::vector<bitmap>,6> shapes;
public:
 explicit shell_animation_artwork(resource_store const &r,std::string prefix="res32/"):resources{r},directory{std::move(prefix)}{}
 void apply(std::vector<shell_animation_command> const &commands,framebuffer &visible,framebuffer &background);
};
}
