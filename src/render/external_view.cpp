#include "external_view.h"

namespace earthsiege {
external_view_artwork::external_view_artwork(resource_store const &r)
 :face{read_font(r.read("simvol0","dfn/GREEN6X8.DFN"))}{
  auto const strings{read_interface_strings(r.read("simvol0","str/STRINGS0.STR"))};
  labels=strings.at(32);player_label=strings.at(14).at(0).text;
}
screen_mask external_view_artwork::clipping(rectangle area){
  screen_mask result;screen_region region{.first_row{area.top},.spans{},.rectangular{true}};
  for(int y{area.top};y<=area.bottom;++y){
    region.spans.push_back({area.left,area.right});
    if(y>=0&&y<static_cast<int>(screen_height))result.rows[static_cast<size_t>(y)].push_back({area.left,area.right});
  }
  result.regions.push_back(std::move(region));return result;
}
void external_view_artwork::draw(framebuffer &frame,bool camera_control,std::string_view target)const{
  frame.fill({0,0,319,bounds.top-1},63);
  frame.fill({0,bounds.bottom+1,319,199},63);
  int const height{static_cast<int>(face.height)};
  int const baseline{bounds.bottom+((199-bounds.bottom)>>1)-(height>>1)+height};
  frame.text(face,labels[0].text+std::string{target.empty()?std::string_view{player_label}:target},50,baseline);
  frame.text(face,labels[1].text+labels[camera_control?2:3].text,215,baseline);
}
}
