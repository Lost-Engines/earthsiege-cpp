#pragma once
#include "render/clouds.h"
#include "render/ground_marks.h"
#include "render/ground_texture.h"
#include "render/environment_palette.h"
#include "resources/resource_store.h"
namespace earthsiege {
struct environment_detail {
 bool panorama{true};
 uint8_t clouds{255},ground{255};
 bool ground_texture{true};
};
/// Original 0x25b68 layer order and shared field/palette lifetime.
class environment_renderer {
 world_environment environment;
 std::vector<bitmap> horizon;
 cloud_field clouds;
 ground_marks marks;
 ground_texture texture;
 environment_palette reflection;
public:
 environment_renderer(resource_store const &,world_environment const &);
 void initialize_palette(palette &colours,uint16_t pitch) { reflection.update(colours,pitch); }
 void draw(framebuffer &,vec3<int32_t> const &,vec3<uint16_t> const &,camera_projection const &,
           rectangle,random_state &,palette &,environment_detail const &detail = {},vec2<int32_t> const &offset={0,0},bool impact=false,bool reflection_enabled=true);
};
}
