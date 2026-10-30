#pragma once
#include "resources/artwork.h"
#include <optional>
#include <utility>
namespace earthsiege {
/// DBSIM 27f74 / 88684: CPU palette and displayed VGA DAC are distinct.
class palette_device {
  palette displayed;
  palette const *source{};
  std::optional<std::pair<unsigned,unsigned>> dirty;
  unsigned next{};
  void upload(palette const &,unsigned first,unsigned count);
public:
  void changed(unsigned first,unsigned count);
  void present(palette const &);
  palette const &colours()const{return displayed;}
  unsigned next_index()const{return next;}
};
}
