#pragma once
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "resources/career.h"
#include "resources/resource_store.h"
namespace earthsiege {
struct return_presentation;
/// GO's specialized career (1231) and mission-return (1522) panels.
class shell_statistics {
 font values,labels;
 std::vector<std::string> strings;
public:
 explicit shell_statistics(resource_store const &);
 void career(framebuffer &,rectangle const &,career_state const &,std::string const &name)const;
 void debrief(framebuffer &,rectangle const &,career_state const &,return_presentation const &)const;
};
}
