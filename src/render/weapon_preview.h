#pragma once
#include "render/shell_animation_artwork.h"
namespace earthsiege {
/// Original one-turn W1800 animation, which commits its final frame.
class weapon_preview {
 shell_animation player;
 shell_animation_artwork art;
 framebuffer background,visible;
public:
 weapon_preview(resource_store const &,int index,framebuffer const &base,random_state &);
 void advance(int16_t ticks,random_state &);
 void draw(framebuffer &,rectangle bounds)const;
};
}
