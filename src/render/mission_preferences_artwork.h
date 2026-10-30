#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
namespace earthsiege {
class mission_preferences_artwork {
 bitmap background;
 std::vector<bitmap> corners;
 std::array<button_style,4> styles;
 font title,metrics,values;
 std::array<font,3> faces;
 interface_strings strings;
public:
 explicit mission_preferences_artwork(resource_store const &);
 rectangle button(unsigned)const;
 void draw(framebuffer &,std::span<uint8_t const,7> values,std::span<uint8_t const,9> faces)const;
};
}
