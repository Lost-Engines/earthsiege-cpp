#pragma once
#include "simulation/random_state.h"
#include <string>
#include <vector>
namespace earthsiege {
struct speech_selection {
 std::string text_file,list_file;
 uint16_t entry{};
 unsigned region{},destination{};
 int song{-1};
};
int chapter_commander(int chapter);
std::vector<uint16_t> chapter_introduction(int chapter);
/// GO 0x37edd. For mode 2 entry is the introduction ordinal; for mode 7 it
/// is the mechanic line selected by the return owner.
speech_selection select_shell_speech(int mode,int chapter,int entry,int grade,int kind,bool experienced,random_state &);
}
