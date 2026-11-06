#pragma once
#include "resources/packed_strings.h"
#include "render/framebuffer.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"
#include "shell/construction_menu.h"
namespace earthsiege {
class construction_artwork {
 std::vector<bitmap> blueprints;
 std::array<font,3> faces;
 shell_text text;
public:
 explicit construction_artwork(resource_store const &);
 void draw(framebuffer &,construction_menu const &,hangar const &,career_state const &,service_prices const &)const;
};
}
