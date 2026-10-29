#include "preferences.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace earthsiege {
preferences::preferences(bytes source,bool live){
  if(!source.empty()){
    if(source.size()!=current_.size())throw std::runtime_error{"PREFS.CFG must contain 17 bytes"};
    std::ranges::copy(source,current_.begin());
  }
  saved_=previous_=current_;
  for(size_t i{};i<current_.size();++i)changed(i,live);
}
void preferences::changed(size_t i,bool live){if(live)changes_.push_back({static_cast<uint8_t>(i),current_.at(i)});}
void preferences::set(size_t i,uint8_t value,bool live){previous_.at(i)=current_.at(i);current_.at(i)=value;changed(i,live);}
void preferences::toggle(size_t i,bool live){set(i,static_cast<uint8_t>(current_.at(i)^1),live);}
void preferences::increase(size_t i,uint8_t amount,bool live){auto const v{current_.at(i)};set(i,v==255?0:static_cast<uint8_t>(std::min(255,int(v)+amount)),live);}
void preferences::decrease(size_t i,uint8_t amount,bool live){auto const v{current_.at(i)};set(i,v==0?255:static_cast<uint8_t>(std::max(0,int(v)-amount)),live);}
void preferences::restore(size_t i,bool live){set(i,saved_.at(i),live);}
void preferences::commit(bool live){for(size_t i{};i<current_.size();++i)if(current_[i]!=saved_[i])changed(i,live);saved_=previous_=current_;}
void preferences::cancel(bool live){for(size_t i{};i<current_.size();++i)if(current_[i]!=saved_[i]){current_[i]=saved_[i];changed(i,live);}}
preferences::values preferences::save_selected(std::optional<bytes> existing,std::span<uint8_t const> indices)const{
  values result{current_};
  if(existing){if(existing->size()!=result.size())throw std::runtime_error{"PREFS.CFG must contain 17 bytes"};std::ranges::copy(*existing,result.begin());}
  for(auto i:indices)result.at(i)=current_.at(i);
  return result;
}
std::vector<preference_change> preferences::take_changes(){return std::exchange(changes_,{});}
} // namespace earthsiege
