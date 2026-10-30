#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
namespace earthsiege {
/// Original GO 0x695f CTL_ALRT window and value fields.
class control_settings_artwork {
 bitmap background;
 std::vector<bitmap> corners;
 std::array<button_style,4> styles;
 font title,metrics,values;
 std::array<font,3> faces;
 interface_strings strings;
 int origin_x{49},origin_y{50};uint8_t field_background{31};
 int width{},title_padding{},title_top{},title_height{},title_offset{};
 std::array<rectangle,7> buttons;
 std::array<int,7> offsets{};
 std::array<rectangle,5> fields;
 int field_x{},field_y{};
public:
 explicit control_settings_artwork(resource_store const &,bool in_flight=false);
 rectangle button(unsigned i)const{return buttons.at(i);}
 void draw(framebuffer &,std::span<uint8_t const,5> state,std::span<uint8_t const,7> appearance)const;
};
}
