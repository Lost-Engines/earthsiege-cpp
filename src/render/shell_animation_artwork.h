#pragma once
#include "render/framebuffer.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"
#include "shell/shell_animation.h"
#include <array>
#include <optional>
namespace earthsiege {
struct shell_palette_frame {framebuffer image;palette colours;};
/// GO brush banks and retained background updates, separate from scheduling.
class shell_animation_artwork {
 resource_store const &resources;
 std::string directory;
 std::array<std::vector<bitmap>,6> shapes;
 std::array<std::optional<palette>,6> palettes;
 uint8_t palette_bank{},pen{10},brush{10};
 palette colours_;
 framebuffer presented;
 bool initialized{},palette_dirty{};
 std::vector<shell_palette_frame> frames;
 void submit(palette const &value);
public:
 explicit shell_animation_artwork(resource_store const &r,std::string prefix="res32/"):resources{r},directory{std::move(prefix)}{}
 palette const &colours()const{return colours_;}
 std::vector<shell_palette_frame> const &palette_frames()const{return frames;}
 void apply(std::vector<shell_animation_command> const &commands,framebuffer &visible,framebuffer &background);
};
}
