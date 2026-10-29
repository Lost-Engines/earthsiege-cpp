#include "dialog.h"
#include <algorithm>
#include <stdexcept>
#include "resources/resource_store.h"
#include "resources/packed_strings.h"

namespace earthsiege {
namespace {


} // namespace

dialog::dialog(resource_store const &resources, unsigned int requested, unsigned int region) :
  backdrop{read_bitmaps(resources.read("shell0", "res32/regn" + std::to_string(region) + ".dbm")).front()},
  corners{read_bitmaps(resources.read("shell0", "res32/corners.dba"))},
  title_face{read_font(resources.read("shell0", "dfn/title.dfn"))},
  caption_face{read_font(resources.read("shell0","dfn/gray.dfn"))},
  button_faces{
    read_font(resources.read("shell0", "dfn/active.dfn")),
    read_font(resources.read("shell0", "dfn/pushed.dfn")),
    read_font(resources.read("shell0", "dfn/inactive.dfn")),
  },
  list_faces{read_font(resources.read("shell0",requested==1610?"dfn/nm_white.dfn":"dfn/nm_green.dfn")),read_font(resources.read("shell0","dfn/nm_red1.dfn"))},
  colours{read_palette(resources.read("shell0", "res32/regn" + std::to_string(region) + ".dpl"))},
  number{requested} {
  if(number==1610)colours=read_palette(resources.read("shell0","res32/regn1610.dpl"));
  auto const artwork{"res32/menu" + std::to_string(number) + ".dba"};
  if(resources.archives().at("shell0").find(artwork))panel=read_bitmaps(resources.read("shell0",artwork)).front();
  auto const labels{read_packed_strings(resources.read("shell0", "eng/menu" + std::to_string(number) + ".bin"))};
  binary_reader file{resources.read("shell0", "dlg/menu" + std::to_string(number) + ".dlg"), "dialog controls"};
  auto const count{file.integer<uint16_t>()};
  if(!count || count > 1000) file.fail("invalid dialog control count");
  for(unsigned int index{}; index < count; ++index) {
    control entry{
      .bounds{
        .left{file.integer<int16_t>()},
        .top{file.integer<int16_t>()},
        .right{file.integer<int16_t>()},
        .bottom{file.integer<int16_t>()},
      },
      .type{file.integer<uint8_t>()},
      .style{file.integer<uint8_t>()},
      .key_scan{file.integer<uint8_t>()},
      .action{file.integer<uint16_t>()},
      .label{index < labels.size() ? labels[index] : ""},
      .items{},
    };
    if(entry.type==3 && entry.style==1 && entry.key_scan)entry.label.clear();
    if(index) {
      entry.bounds.left += controls.front().bounds.left;
      entry.bounds.right += controls.front().bounds.left;
      entry.bounds.top += controls.front().bounds.top;
      entry.bounds.bottom += controls.front().bounds.top;
    }
    if(entry.type==2){entry.label.clear();text_focus=index;}
    controls.push_back(std::move(entry));
  }
  file.require_end();
  styles=read_button_styles(resources.read("shell0","cfg/styles.dat"));
}

void dialog::draw(framebuffer &frame, bool background) const {
  if(background)frame.image(backdrop, 0, 0);
  auto const &root{controls.front()};
  frame.image(panel, root.bounds.left, root.bounds.top, true);
  if(root.style==0) {
  // GO 0x36b4c centres the caption inside a 40-pixel padded, 17-pixel plaque
  int const caption_width{title_face.text_width(root.label) + 40};
  int const caption_left{root.bounds.left + (root.bounds.right - root.bounds.left - caption_width) / 2};
  rectangle const caption{
    .left{caption_left},
    .top{root.bounds.top + 5},
    .right{caption_left + caption_width},
    .bottom{root.bounds.top + 22},
  };
  frame.line(caption.left, caption.top, caption.right, caption.top, 16);
  frame.line(caption.left, caption.top, caption.left, caption.bottom, 16);
  frame.line(caption.right, caption.top, caption.right, caption.bottom, 27);
  frame.line(caption.left, caption.bottom, caption.right, caption.bottom, 28);
  frame.image(corners.at(0), caption.left, caption.top, true);
  frame.image(corners.at(1), caption.right - 4, caption.top, true);
  frame.image(corners.at(2), caption.left, caption.bottom - 4, true);
  frame.image(corners.at(3), caption.right - 4, caption.bottom - 4, true);
  frame.text(title_face, root.label, caption.left + (caption_width - title_face.text_width(root.label)) / 2, caption.top + 7);
  }
  for(unsigned int index{1}; index < controls.size(); ++index) {
    auto const &entry{controls[index]};
    if(entry.type == 15 || entry.type == 2) draw_dialog_button(frame, entry.bounds, styles[3], corners);
    if(entry.type==15 || entry.type==2 || entry.type==1 || entry.type==4 || entry.type==5) {
      auto bounds{entry.bounds};bounds.left+=1;bounds.right-=1;bounds.top+=1;bounds.bottom-=1;
      screen_mask mask;
      for(int y{std::max(0,bounds.top)};y<=std::min(199,bounds.bottom);++y)mask.rows[static_cast<size_t>(y)].push_back({bounds.left,bounds.right});
      scoped_framebuffer_clip clip{frame,&mask};
      if(entry.type==15) {
        int const height{row_height(entry)};
        int const visible{entry.style ? 1 : std::max(1,(entry.bounds.bottom-entry.bounds.top)/height)};
        int const first{entry.style ? entry.selection : entry.first_row};
        for(int row{};row<visible && first+row<static_cast<int>(entry.items.size());++row) {
          auto const &text{entry.items[static_cast<size_t>(first+row)]};
          bool const selected{entry.style==0 && first+row==entry.selection};
          auto const &face{list_faces[selected ? 1 : 0]};
          int const y{entry.bounds.top+row*height};
          if(selected){frame.fill({entry.bounds.left+1,y+1,entry.bounds.right-1,y+height},4);frame.line(entry.bounds.left+1,y+1,entry.bounds.right-1,y+1,5);}
          auto const x{entry.style?(entry.bounds.left+entry.bounds.right-face.text_width(text))/2:entry.bounds.left+3};
          frame.text(face,text,x,y+height-1);
        }
      } else {
        auto const &face{(number==1900||number==1920)&&(entry.type==1||entry.type==5)?caption_face:list_faces[0]};
        int const x{entry.type==5 ? (bounds.left+bounds.right-face.text_width(entry.label))/2 : bounds.left};
        frame.text(face,entry.label,x,bounds.top+face.baseline);
        if(entry.type==2 && text_focus==index)frame.line(x+face.text_width(entry.label),bounds.top,x+face.text_width(entry.label),bounds.bottom,0x22);
      }
    }
    if(number==1910&&entry.type==16){auto b{entry.bounds};frame.fill(b,entry.selection?0x24:0x1d);frame.line(b.left,b.top,b.right,b.top,0x1b);frame.line(b.left,b.top,b.left,b.bottom,0x1b);}
    if(entry.type != 3) continue;
    bool const disabled{!entry.enabled};
    unsigned int const state{disabled ? 2u : pressed == index ? 1u : 0u};
    // GO 0x1d47d gives these paired controls one shared 56-pixel sprite
    // and text anchor. Draw it once, with either half's pressed state.
    bool const paired{entry.style==1 && !entry.key_scan &&
      (entry.action==1806 || entry.action==1807 || entry.action==2003 || entry.action==2004)};
    if(paired) {
      bool const right{entry.action==1807 || entry.action==2004};
      auto const other{find(static_cast<uint16_t>(entry.action+(right?-1:1)))};
      if(right && other)continue;
      auto const active{pressed?&controls[*pressed]:nullptr};
      bool const pushed{active==&entry || (other && active==other)};
      unsigned const pair_state{disabled?2u:pushed?1u:0u};
      bool const right_down{active && (active->action==1807 || active->action==2004)};
      auto const &sprite{corners.at(disabled?51u:pushed?(right_down?49u:50u):48u)};
      auto const &face{button_faces[pair_state]};
      int const x{entry.bounds.left-(right?27:1)};
      int const y{entry.bounds.bottom-((entry.bounds.bottom-entry.bounds.top)-static_cast<int>(face.height))/2-2};
      frame.image(sprite,x,entry.bounds.top-1,true);
      frame.text(face,entry.label,x+(static_cast<int>(sprite.width)-face.text_width(entry.label))/2,y);
      continue;
    }
    auto const &face{button_faces[state]};
    if(entry.style==1 && entry.label.empty()) {
      // Directional controls use the original icon sheet, not a host font.
      unsigned icon{20},down{22};
      switch(entry.key_scan) {
        case 80:case 81:icon=21;down=23;break;
        case 75:icon=25;down=27;break;
        case 77:icon=24;down=26;break;
        case 78:icon=28;down=30;if(entry.action==108){icon=32;down=33;}break;
        case 74:icon=29;down=31;if(entry.action==109){icon=34;down=35;}break;
        default:break;
      }
      if(entry.action==2003){icon=44;down=46;}
      if(entry.action==2004){icon=45;down=47;}
      if(entry.action>=1503&&entry.action<=1506){icon=36+(entry.action-1503)*2;down=icon+1;}
      auto const &sprite{corners.at(state==1?down:icon)};
      frame.image(sprite,entry.bounds.left,entry.bounds.top,true);
      continue;
    }
    draw_dialog_button(frame, entry.bounds, styles[state], corners);
    frame.text(face, entry.label, (entry.bounds.left + entry.bounds.right - face.text_width(entry.label)) / 2, entry.bounds.bottom - ((entry.bounds.bottom - entry.bounds.top) - static_cast<int>(face.height)) / 2 - 2 + (state == 1 ? 1 : 0));
  }
}

void dialog::mouse_down(int x, int y) {
  pressed.reset();
  for(unsigned int index{1}; index < controls.size(); ++index) {
    auto const &entry{controls[index]};
    if(entry.enabled && entry.bounds.contains(x,y)) {
      if(entry.type==3 || entry.type==16 || (entry.type==15 && entry.style==0))pressed=index;
      if(entry.type==2)text_focus=index;
    }
  }
}

auto dialog::mouse_up(int x, int y)->std::optional<uint16_t> {
  auto const previous{pressed};
  pressed.reset();
  if(previous && controls[*previous].bounds.contains(x,y)) {
    auto &entry{controls[*previous]};
    if(entry.type==15){entry.selection=entry.first_row+std::max(0,(y-entry.bounds.top-2)/row_height(entry));reveal(entry);}
    return entry.action;
  }
  return {};
}

control *dialog::find(uint16_t action) {
  auto it{std::ranges::find(controls,action,&control::action)};return it==controls.end()?nullptr:&*it;
}
control const *dialog::find(uint16_t action) const {
  auto it{std::ranges::find(controls,action,&control::action)};return it==controls.end()?nullptr:&*it;
}
int dialog::row_height(control const &entry) const {return std::max(1,std::min(static_cast<int>(list_faces[0].height)-1,entry.bounds.bottom-entry.bounds.top-1));}
void dialog::reveal(control &entry) {
  entry.selection=std::clamp(entry.selection,0,std::max(0,static_cast<int>(entry.items.size())-1));
  int const visible{std::max(1,(entry.bounds.bottom-entry.bounds.top)/row_height(entry))};
  if(entry.selection<entry.first_row)entry.first_row=entry.selection;
  if(entry.selection>=entry.first_row+visible)entry.first_row=entry.selection-visible+1;
}
void dialog::replace_text(std::string_view from,std::string_view to){
  for(auto &c:controls)if(auto pos{c.label.find(from)};pos!=std::string::npos)c.label.replace(pos,from.size(),to);
}
void dialog::enable(uint16_t action,bool enabled) {if(auto c{find(action)})c->enabled=enabled;}
void dialog::items(uint16_t action,std::vector<std::string> values,int selected) {
  if(auto c{find(action)}){c->items=std::move(values);c->selection=selected;c->first_row=0;reveal(*c);}
}
void dialog::select(uint16_t action,int selected) {if(auto c{find(action)}){c->selection=selected;reveal(*c);}}
int dialog::selection(uint16_t action) const {auto c{find(action)};return c?c->selection:0;}
std::optional<uint16_t> dialog::shortcut(uint8_t scan) const {
  if(scan)for(auto const &c:controls)if(c.type==3 && c.enabled && c.key_scan==scan)return c.action;
  return {};
}
void dialog::text_input(std::string_view text) {
  if(!text_focus)return;
  auto &c{controls[*text_focus]};
  for(unsigned char ch:text)if(ch>=32&&ch<127&&c.label.size()<63) {
    auto next{c.label+static_cast<char>(ch)};
    if(list_faces[0].text_width(next)<c.bounds.right-c.bounds.left-5)c.label=std::move(next);
  }
}
void dialog::backspace(){if(text_focus&&!controls[*text_focus].label.empty())controls[*text_focus].label.pop_back();}

void dialog::cancel_press() { pressed.reset(); }
auto dialog::current_palette() const->palette const & { return colours; }
auto dialog::id() const->unsigned int { return number; }

} // namespace earthsiege
