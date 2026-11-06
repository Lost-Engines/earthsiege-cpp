#include "presentation_player.h"
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {
presentation_player::presentation_player(resource_store const &source, std::string volume_name, std::string const &path, uint32_t const seed)
  : resources{source}, volume{std::move(volume_name)}, directory{path.substr(0, path.find_last_of('/') + 1)}, program{read_presentation(resources.read(volume, path))} {
  current.random = seed;
}
void presentation_player::select_label(int16_t const label) {
  current.page = program.label_page(label); current.remaining = 0; current.wait_latched = false;
}
presentation_state const &presentation_player::state() const { return current; }
void presentation_player::execute() {
  if(current.page >= program.pages.size()) throw std::runtime_error{"TTM page outside program"};
  for(auto const &instruction : program.pages[current.page]) {
    auto const &a{instruction.arguments};
    switch(instruction.opcode) {
    case 0x0ff0: return;
    case 0x1101: case 0x1111: break; // Local/global label declarations.
    case 0x0110: current.mode = 3; break;
    case 0x1061: break; // APOCA selects an empty palette bank; mission lamp scripts load none.
    case 0x0400: break; // Ends palette cycling; cockpit scripts have no palette cycles.
    case 0x1051:
      selected_bank = static_cast<uint8_t>(a.at(0));
      if(selected_bank >= banks.size()) throw std::runtime_error{"TTM shape bank outside range"};
      break;
    case 0xf02f:
      if(current.remaining <= 0) {
        auto path{normalise_path(instruction.text)};
        // Original 0x79b54 maps authoring filenames to ThreeSpace resources.
        if(path.ends_with(".bmp")) path.replace(path.size() - 4, 4, ".dba");
        banks[selected_bank] = std::make_shared<shape_bank>(read_bitmaps(resources.read(volume, directory + path)));
      }
      break;
    case 0x1021: case 0x2022:
      if(!current.wait_latched) {
        int16_t wait{a.at(0)};
        if(instruction.opcode == 0x2022) {
          int const range{a.at(1) - a.at(0)};
          if(range <= 0) throw std::runtime_error{"TTM random wait has an empty range"};
          current.random = current.random * 0x41c64e6du + 0x3039u;
          wait = wrap_word(a.at(0) + static_cast<int>((current.random >> 16) & 0x7fff) % range);
        }
        current.wait_latched = true; current.period = current.remaining = wait;
      }
      break;
    case 0x1201: current.jump = program.label_page(a.at(0)); break;
    case 0xa504: case 0xa514: case 0xa524: case 0xa534: {
      auto const &bank{banks.at(static_cast<size_t>(a.at(3)))};
      if(bank && a.at(2) >= 0 && static_cast<size_t>(a.at(2)) < bank->size())
        brushes.push_back({bank, static_cast<size_t>(a.at(2)), a.at(0), a.at(1), static_cast<unsigned int>((instruction.opcode >> 4) & 3)});
      break;
    }
    default: throw std::runtime_error{"TTM operation not yet implemented: " + std::to_string(instruction.opcode)};
    }
  }
}
void presentation_player::advance(int16_t const elapsed) {
  brushes.clear();
  current.remaining = wrap_word(current.remaining - elapsed);
  if(current.remaining < 1) {
    current.remaining = current.period;
    switch(current.mode) {
    case 0:
      execute();
      if(current.jump == -1) { ++current.page; current.wait_latched = false; }
      else { current.page = static_cast<uint8_t>(current.jump); current.jump = -1; }
      if(current.page >= program.pages.size()) current.mode = 2;
      break;
    case 1: execute(); break;
    case 3: current.mode = 2; break;
    default: break;
    }
  } else if(current.mode != 2) execute();
}
void presentation_player::draw(framebuffer &target, int const offset_x, int const offset_y) const {
  for(auto const &brush : brushes) {
    auto const &image{brush.shapes->at(brush.frame)};
    for(unsigned int y{}; y < image.height; ++y) for(unsigned int x{}; x < image.width; ++x) {
      auto const source_x{brush.flip & 2 ? image.width - x - 1 : x};
      auto const source_y{brush.flip & 1 ? image.height - y - 1 : y};
      auto const colour{image.pixels[source_y * image.width + source_x]};
      if(colour) target.pixel(brush.x + offset_x + static_cast<int>(x), brush.y + offset_y + static_cast<int>(y), colour);
    }
  }
}
} // namespace earthsiege
