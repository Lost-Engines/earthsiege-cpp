#include "wingmate_orders.h"
#include <limits>

namespace earthsiege {
namespace {
int32_t distance(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return fast_distance({wrap_long(static_cast<int64_t>(a.x)-b.x), wrap_long(static_cast<int64_t>(a.y)-b.y), wrap_long(static_cast<int64_t>(a.z)-b.z)});
}
bool incapable(wingmate_order_state const &a) { return a.unavailable || a.disabled || a.dead; }
}
bool receive_wingmate_order(std::span<wingmate_order_state> const actors, size_t const index,
  wingmate_order const &order, uint16_t const previous_reply, size_t const player, size_t const leader,
  int16_t const group_order, std::optional<size_t> const hud_target, wingmate_order_callbacks const &callbacks) {
  auto &a{actors[index]};
  bool accepted{}; uint16_t reply{0xffff};
  auto const props{[&]{return describe(a.type).properties;}};
  auto const attack{[&](size_t target) {
    a.command=4; a.designated=target;
    auto const properties{describe(actors[target].type).properties};
    a.stop_status=properties&32 ? 2 : properties&16 ? 1 : 0;
    callbacks.attack(target); a.incoming_damage=0;
  }};
  switch(order.command) {
    case 0:
      a.ignore_target=false;
      if(incapable(a)) reply=27;
      else if(props()&16) reply=13;
      else if(!hud_target || actors[*hud_target].dead || actors[*hud_target].disabled) reply=18;
      else if(distance(a.position,actors[*hud_target].position)>=130001) reply=16;
      else if(a.command==4 && a.designated==hud_target) reply=15;
      else {attack(*hud_target); accepted=true; reply=17;}
      break;
    case 1:
      if(incapable(a)) reply=30;
      else if(!hud_target) reply=18;
      else if(a.ignore_target) reply=15;
      else {
        a.ignore_target=true; accepted=true; reply=22;
        if((props()&2) && a.selected==hud_target) {
          if(props()&4) a.reconsider_remaining=0;
          else if(auto const target{callbacks.select(true)}) callbacks.attack(*target);
          else callbacks.resume();
        }
      }
      break;
    case 2:
      if(incapable(a)) reply=27;
      else {
        std::optional<size_t> nearest; int32_t range{60000};
        for(size_t i{}; i<actors.size(); ++i) {
          auto const &candidate{actors[i]};
          if(!candidate.herc || incapable(candidate) || candidate.selected!=player) continue;
          auto const d{distance(candidate.position,actors[player].position)};
          if(d<range) {range=d; nearest=i;}
        }
        if(!nearest) reply=29;
        else if(a.selected==nearest && (props()&2)) reply=26;
        else {attack(*nearest); accepted=true; reply=11;}
      }
      break;
    case 3: case 10: case 15:
      if(a.disabled) reply=27;
      else {
        reply=distance(actors[leader].position,a.position)<25000 ? 30 : 20;
        callbacks.enter(behavior::patrolling); a.command=0; a.incoming_damage=0; accepted=true;
      }
      break;
    case 4: case 16:
      reply=a.radar ? 15 : 38; a.radar=true; a.radar_requested=true; accepted=true;
      break;
    case 5:
      a.ignore_target=false;
      if(incapable(a)) reply=27;
      else if(props()&2) {reply=32; accepted=true;}
      else if(auto const target{callbacks.select(true)}) {
        if(group_order==6) {a.command=4; a.designated.reset(); callbacks.resume();}
        else {a.command=0; callbacks.attack(*target);}
        reply=28; accepted=true;
      } else reply=14;
      break;
    case 7: case 17:
      a.radar=false; a.radar_requested=false; reply=40; accepted=true;
      break;
    case 8:
      if(props()&2) {callbacks.resume(); a.command=0; accepted=true;}
      else accepted=callbacks.select(false).has_value();
      reply=12; a.incoming_damage=0;
      break;
    case 11:
      a.ignore_target=false;
      if(incapable(a)) reply=27;
      else if(a.command==4 && a.designated==order.target) reply=15;
      else if(!order.target || actors[*order.target].disabled || actors[*order.target].dead) reply=29;
      else {attack(*order.target); reply=17; accepted=true;}
      break;
    case 12:
      if(incapable(a)) reply=27;
      else if(a.command==6 && fast_distance_2d({wrap_long(static_cast<int64_t>(order.position.x)-a.command_position.x),wrap_long(static_cast<int64_t>(order.position.y)-a.command_position.y)})<1000) reply=32;
      else {callbacks.enter(behavior::guarding); a.command=6; reply=23; accepted=true;}
      a.command_position={order.position.x,order.position.y};
      break;
    case 13: case 14: {
      int16_t const command{order.command==13 ? int16_t{2} : int16_t{1}};
      if(a.disabled) reply=27;
      else if(a.command==command && distance(a.position,order.position)<2000) reply=32;
      else {callbacks.enter(behavior::patrolling); a.command=command; reply=42; accepted=true;}
      a.command_position={order.position.x,order.position.y};
      break;
    }
    default: break;
  }
  if(previous_reply==0 || (previous_reply==1 && accepted)) callbacks.acknowledge(reply);
  return accepted;
}

bool dispatch_wingmate_order(std::span<wingmate_order_state const> const actors,
  std::span<size_t const> const group, wingmate_order const &order,
  std::function<bool(size_t,uint16_t)> const &deliver, int16_t const fallback_priority) {
  std::vector<bool> visited(group.size()); uint16_t replies{};
  for(;;) {
    std::optional<size_t> best; int priority{-1}; int32_t range{std::numeric_limits<int32_t>::max()};
    for(size_t i{}; i<group.size(); ++i) {
      auto const index{group[i]}; auto const &a{actors[index]};
      if(visited[i] || a.dead || index==order.issuer) continue;
      bool const attacking{describe(a.type).attacking()}; int candidate{fallback_priority};
      switch(order.command) {
        case 0: case 2: case 5: candidate=!attacking; break;
        case 1: case 8: case 15: candidate=attacking; break;
        case 4: candidate=!a.radar; break;
        case 7: candidate=a.radar; break;
        default: break;
      }
      if(candidate<priority) continue;
      auto const d{distance(a.position,actors[order.issuer].position)};
      if(d<range || candidate>priority) {best=i; priority=candidate; range=d;}
    }
    if(!best) return replies==2;
    bool const accepted{deliver(group[*best],replies)}; visited[*best]=true;
    if(accepted) replies=2; else if(replies==0) replies=1;
    if(accepted && (order.command==0 || order.command==2)) return true;
  }
}
} // namespace earthsiege
