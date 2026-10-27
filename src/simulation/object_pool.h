#pragma once
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace earthsiege {

/// Fixed capacity with original linked ordering and deferred slot reuse.
/// Objects own their resources; queued objects remain allocated until collect().
template<typename T> class object_pool {
  static size_t constexpr end{std::numeric_limits<size_t>::max()};
  struct slot {
    std::optional<T> value;
    size_t next{end};
    size_t previous{end};
    bool retiring{false};
  };
  std::vector<slot> slots;
  std::vector<size_t> pending;
  size_t free_head{end};
  size_t active_head{end};
  size_t free_count{0};

public:
  explicit object_pool(size_t const capacity) : slots(capacity), free_head{capacity == 0 ? end : 0}, free_count{capacity} {
    for(size_t i{0}; i < capacity; ++i) slots[i].next = i + 1 < capacity ? i + 1 : end;
  }
  object_pool(object_pool const &) = delete;
  object_pool &operator=(object_pool const &) = delete;
  object_pool(object_pool &&) = delete;
  object_pool &operator=(object_pool &&) = delete;
  size_t available() const { return free_count; }
  size_t size() const { return slots.size() - free_count; }
  T &at(size_t const index) { return slots.at(index).value.value(); }
  T const &at(size_t const index) const { return slots.at(index).value.value(); }
  bool retiring(size_t const index) const { return slots.at(index).retiring; }

  template<typename... Args> std::optional<size_t> emplace(Args &&...args) {
    if(free_head == end) return std::nullopt;
    auto const index{free_head}; auto &entry{slots[index]};
    entry.value.emplace(std::forward<Args>(args)...);
    free_head = entry.next;
    entry.next = active_head; entry.previous = end; entry.retiring = false;
    if(active_head != end) slots[active_head].previous = index;
    active_head = index; --free_count;
    return index;
  }

  /// Immediate release is distinct from the original end-of-phase collection.
  void erase(size_t const index) {
    auto &entry{slots.at(index)};
    if(!entry.value || entry.retiring) throw std::logic_error{"invalid direct pool release"};
    if(entry.previous == end) active_head = entry.next;
    else slots[entry.previous].next = entry.next;
    if(entry.next != end) slots[entry.next].previous = entry.previous;
    entry.value.reset();
    entry.next = free_head; entry.previous = end;
    free_head = index; ++free_count;
  }

  void defer_erase(size_t const index) {
    auto &entry{slots.at(index)};
    if(!entry.value || entry.retiring) throw std::logic_error{"invalid deferred pool release"};
    pending.push_back(index);
    entry.retiring = true;
  }

  template<typename Cleanup> void collect(Cleanup &&cleanup) {
    for(auto const index : pending) {
      cleanup(index, *slots[index].value);
      slots[index].retiring = false;
      erase(index);
    }
    pending.clear();
  }
  void collect() { collect([](size_t, T &){}); }

  /// Visitors may queue removal; they must not erase or collect during traversal.
  template<typename Visitor> void for_each(Visitor &&visitor) {
    for(auto index{active_head}; index != end; index = slots[index].next) visitor(index, *slots[index].value);
  }
  /// The original simulation consumer walks from the oldest entry toward the head.
  template<typename Visitor> void for_each_oldest(Visitor &&visitor) {
    auto index{active_head};
    if(index == end) return;
    while(slots[index].next != end) index = slots[index].next;
    while(index != end) { visitor(index, *slots[index].value); index = slots[index].previous; }
  }
  template<typename Visitor> void for_each(Visitor &&visitor) const {
    for(auto index{active_head}; index != end; index = slots[index].next) visitor(index, *slots[index].value);
  }
};

} // namespace earthsiege
