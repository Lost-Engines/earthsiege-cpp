#pragma once
#include <array>
#include <string>
#include <vector>
#include "simulation/random_state.h"
namespace earthsiege {
struct speech_selection {
  std::string text_file;
  std::string list_file;
  uint16_t entry{0};
  unsigned region{0};
  unsigned destination{0};
  int song{-1};
};
int chapter_commander(int chapter, bool expansion = false);
std::vector<uint16_t> chapter_introduction(int chapter, bool expansion = false);
/// GO 0x37edd. For mode 2 entry is the introduction ordinal; for mode 7 it
/// is the mechanic line selected by the return owner.
speech_selection select_shell_speech(int mode, int chapter, int entry, int grade, int kind, bool experienced, random_state &, bool expansion = false);
std::array<int16_t, 3> select_mechanic_return(int condition, bool technology, int32_t recovery, int deaths, random_state &random);
} // namespace earthsiege
