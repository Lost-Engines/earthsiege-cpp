#include "mfd_controller.h"
#include <stdexcept>
#include <utility>

namespace earthsiege {
mfd_controller::mfd_controller(size_t const player, uint32_t const ticks) {
  data.selected_actor = player;
  data.deadline = ticks + 30;
}
mfd_state const &mfd_controller::state() const { return data; }
void mfd_controller::select(mfd_page const page, size_t const player, std::optional<size_t> const target) {
  if(std::to_underlying(page) >= data.refresh.size()) throw std::out_of_range{"MFD page"};
  if(data.page == page) return;
  data.page = page;
  if(data.automatic_camera) { data.allow_camera_switch = false; data.automatic_camera = false; }
  data.pending_paint = 2;
  if(page == mfd_page::status) data.selected_actor = player;
  else if(page == mfd_page::target) data.selected_actor = target;
}
void mfd_controller::request_update() { data.refresh[std::to_underlying(data.page)] = true; }
void mfd_controller::select_choice(size_t const choice) {
  if(choice >= 6) throw std::out_of_range{"communications choice"};
  data.choice = choice;
  data.refresh[std::to_underlying(mfd_page::communications)] = true;
}
mfd_action mfd_controller::button(size_t const index, size_t const player, std::optional<size_t> const target, std::span<communications_choice const, 6> choices) {
  if(index < 6) select(static_cast<mfd_page>(index), player, target);
  else if(index == 6) select(next_mfd_page(data.page), player, target);
  else if(index == 7 && data.page == mfd_page::scanner) return mfd_action::toggle_radar;
  else if(index == 8) { data.range_index = next_mfd_range(data.range_index); request_update(); }
  else if(index == 9) { request_update(); return mfd_action::cycle_target; }
  else if(index == 10 && data.page == mfd_page::communications) { request_update(); return mfd_action::transmit; }
  else if((index == 11 || index == 12) && data.page == mfd_page::communications) {
    size_t candidate{data.choice};
    for(size_t count{}; count < choices.size(); ++count) {
      candidate = (candidate + (index == 12 ? 1 : 5)) % choices.size();
      if(!choices[candidate].disabled) { select_choice(candidate); break; }
    }
  }
  return mfd_action::none;
}
std::vector<mfd_paint> mfd_controller::advance(uint32_t const ticks, bool const visible, bool const external_camera, bool const eo_selected,
                                               size_t const player, std::optional<size_t> const target) {
  std::vector<mfd_paint> paints;
  if(external_camera || !visible) return paints;
  if(!data.automatic_camera && eo_selected) {
    if(data.allow_camera_switch) {
      data.previous_page = data.page;
      select(mfd_page::missile_camera, player, target);
    } else data.allow_camera_switch = true;
    data.automatic_camera = true;
  } else if(data.automatic_camera && !eo_selected) {
    select(data.previous_page, player, target);
    data.allow_camera_switch = true;
  }
  if(data.pending_paint == 2) { paints.push_back({data.page, true}); --data.pending_paint; }
  else if(data.pending_paint == 1) { --data.pending_paint; request_update(); }
  auto const index{std::to_underlying(data.page)};
  if(data.refresh[index]) {
    bool paint{true};
    switch(data.page) {
      case mfd_page::status: case mfd_page::navigation: case mfd_page::target:
        if(data.deadline < ticks) {
          data.deadline = ticks + 30;
          if(data.page == mfd_page::status) data.selected_actor = player;
          else if(data.page == mfd_page::target) data.selected_actor = target;
        } else paint = false;
        break;
      case mfd_page::communications: data.refresh[index] = false; break;
      case mfd_page::scanner: case mfd_page::missile_camera: break;
    }
    if(paint) paints.push_back({data.page, false});
  }
  return paints;
}
} // namespace earthsiege
