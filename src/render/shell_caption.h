#pragma once
#include "render/framebuffer.h"
#include "resources/resource_store.h"
#include "resources/artwork.h"
#include <optional>
namespace earthsiege {
class shell_caption {
 font face;
 bitmap panel;
 std::vector<std::optional<std::string>> rows;
 rectangle area;
 int step{},rows_per_page{},pages{},selected{};
 bool banner{};
public:
 shell_caption(resource_store const &,std::string text,unsigned region);
 void next();
 void previous();
 void draw(framebuffer &,uint8_t speech_preference)const;
 rectangle bounds()const{return area;}
 auto const &lines()const{return rows;}
 int page()const{return selected;}
 int page_count()const{return pages;}
 int page_rows()const{return rows_per_page;}
};
}
