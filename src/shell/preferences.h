#pragma once
#include "resources/binary_reader.h"
#include <array>
#include <optional>

namespace earthsiege {
struct preference_change {uint8_t index{},value{};bool operator==(preference_change const &)const=default;};
/// GO/DBSIM share the same 17-byte transaction manager. Disk persistence is
/// separate from accepting an edit: saving must not silently commit it.
class preferences {
public:
  using values=std::array<uint8_t,17>;
private:
  values current_{},saved_{},previous_{};
  std::vector<preference_change> changes_;
  void changed(size_t index,bool live);
public:
  explicit preferences(bytes source={},bool live=true);
  values const &current()const{return current_;}
  values const &saved()const{return saved_;}
  values const &previous()const{return previous_;}
  void set(size_t index,uint8_t value,bool live=true);
  void toggle(size_t index,bool live=true);
  void increase(size_t index,uint8_t amount,bool live=true);
  void decrease(size_t index,uint8_t amount,bool live=true);
  void restore(size_t index,bool live=true);
  void commit(bool live=true);
  void cancel(bool live=true);
  values save_selected(std::optional<bytes> existing,std::span<uint8_t const> indices)const;
  std::vector<preference_change> take_changes();
};
} // namespace earthsiege
