#pragma once
#include <cstdint>
namespace earthsiege {
struct music_loop_state {
  uint16_t configured{},remaining{};
  bool allow(uint8_t branch,bool playing){
    if(!(branch&128))return true;
    auto &count{playing?remaining:configured};if(count==65535)return true;if(!count)return false;--count;return true;
  }
};
}
