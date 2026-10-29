#pragma once
#include "shell/dialog.h"
#include "shell/crew_assignment.h"
namespace earthsiege {
class crew_artwork {
 std::vector<bitmap> portraits,mechs;
 bitmap insignia,mesh;
 std::array<font,3> faces;
 std::vector<std::string> text;
public:
 explicit crew_artwork(resource_store const &);
 void draw(framebuffer &,dialog const &,hangar const &,career_state const &,
           crew_assignment const &,std::string const &player_name)const;
};
}
