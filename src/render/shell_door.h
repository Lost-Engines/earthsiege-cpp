#pragma once
#include "resources/lpf_animation.h"
#include "render/framebuffer.h"
#include "resources/artwork.h"
namespace earthsiege {
/// Short menu transition; no input is delivered through it.
class shell_door {
 lpf_animation animation;
 bitmap frame;
 std::vector<uint8_t> saved;
 palette palette_;
 unsigned next{};
 bool done{};
public:
 shell_door(bytes data,framebuffer const &base,palette const &colours);
 void advance();
 void draw(framebuffer &target)const{target.image(frame,0,0);}
 palette const &colours()const{return palette_;}
 bool finished()const{return done;}
};
}
