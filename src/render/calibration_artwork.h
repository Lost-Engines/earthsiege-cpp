#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
namespace earthsiege {
class calibration_artwork {
 bitmap background;
 std::vector<bitmap> corners;
 std::array<button_style,4> styles;
 font title,metrics,values;
 std::array<font,3> faces;
 interface_strings strings;
 int width{},title_padding{},title_top{},title_height{},title_offset{};
 std::array<rectangle,3> buttons;
 std::array<int,3> offsets{};
 rectangle value_field,prompt;
 int value_x{},value_y{};
public:
 explicit calibration_artwork(resource_store const &);
 rectangle button(unsigned i)const{return buttons.at(i);}
 void draw(framebuffer &,unsigned choice,int message,std::span<uint8_t const,3> appearance)const;
};
}
