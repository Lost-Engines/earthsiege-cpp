#pragma once
#include "resources/binary_reader.h"
namespace earthsiege {
/// GO's LPF/ANM delta records. The closing ring record is not displayed.
class lpf_animation {
 std::vector<std::vector<uint8_t>> records;
 uint16_t width_{},height_{},fps_{};
public:
 explicit lpf_animation(bytes);
 unsigned width()const{return width_;}
 unsigned height()const{return height_;}
 unsigned frames()const{return static_cast<unsigned>(records.size());}
 unsigned frames_per_second()const{return fps_;}
 /// Skip spans retain the prior frame. Written zero pixels use the saved
 /// screen when supplied, as in the original menu-transition span writers.
 void decode(unsigned frame,std::span<uint8_t> target,bytes saved={})const;
};
}
