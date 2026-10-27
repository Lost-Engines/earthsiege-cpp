#include "mission_text.h"
#include <stdexcept>

namespace earthsiege {
namespace {

enum class text_opcode : uint8_t {
  literal = 1,
  counter_choice,
  end = 0x10,
};

auto counted_text(binary_reader &file)->std::string {
  auto const amount{file.integer<uint16_t>()};
  auto const data{file.take(amount)};
  if(data.empty() || data.back()) file.fail("text string lacks final NUL");
  return {reinterpret_cast<char const *>(data.data()), data.size() - 1};
}

auto program(bytes source)->text_program {
  /// GO 0x19453: literal strings, counter choices, then byte 0x10 terminates.
  binary_reader file{source, "mission text program"};
  text_program result{
    .maximum_text{file.integer<uint16_t>()},
    .segments{},
  };
  for(;;) {
    auto const opcode{static_cast<text_opcode>(file.integer<uint8_t>())};
    if(opcode == text_opcode::end) break;
    if(opcode == text_opcode::literal) result.segments.emplace_back(counted_text(file));
    else if(opcode == text_opcode::counter_choice) {
      counter_phrase phrase{
        .counter{file.integer<uint16_t>()},
        .choices{},
      };
      auto const count{file.integer<uint16_t>()};
      if(!count || count > file.remaining() / 6) file.fail("invalid mission text choice count");
      std::vector<int16_t> offsets;
      offsets.reserve(count);
      for(unsigned int index{}; index < count; ++index) offsets.push_back(file.integer<int16_t>());
      std::vector<conditional_phrase> choices;
      choices.reserve(count);
      for(unsigned int index{}; index < count; ++index) {
        choices.push_back(conditional_phrase{
          .first{file.integer<int16_t>()},
          .last{file.integer<int16_t>()},
          .text{},
        });
      }
      auto const base{file.position()};
      if(offsets.back() < 0 || static_cast<unsigned int>(offsets.back()) > file.remaining()) file.fail("text choice end outside section");
      for(unsigned int index{}; index + 1 < count; ++index) {
        if(offsets[index] < 0 || offsets[index] > offsets.back()) file.fail("text alternative outside choice extent");
        binary_reader branch{source.subspan(base, static_cast<unsigned int>(offsets.back())), "conditional phrase"};
        branch.seek(static_cast<unsigned int>(offsets[index]));
        choices[index].text = counted_text(branch);
        phrase.choices.push_back(std::move(choices[index]));
      }
      file.seek(base + static_cast<unsigned int>(offsets.back()));
      result.segments.emplace_back(std::move(phrase));
    } else file.fail("unknown mission text opcode");
  }
  file.require_end();
  return result;
}

} // namespace

auto read_mission_text(bytes source)->mission_text {
  binary_reader file{source, "mission text"};
  if(file.integer<uint16_t>() != 1) file.fail("unsupported mission text revision");
  mission_text result;
  while(file.remaining()) {
    auto const tag{static_cast<mission_text_kind>(file.integer<uint32_t>())};
    auto const data{file.take(file.integer<uint32_t>())};
    if(tag == mission_text_kind::briefing || tag == mission_text_kind::debriefing) {
      result.sections.push_back(mission_text_section{
        .kind{tag},
        .content{program(data)},
      });
    } else if(tag == mission_text_kind::objectives) {
      binary_reader objectives{data, "mission objectives"};
      objectives.take(2);                                                    // maximum concatenated text allocation in the original
      auto const count{objectives.integer<uint16_t>()};
      std::vector<objective_text> entries;
      entries.reserve(count);
      for(unsigned int index{}; index < count; ++index) {
        entries.push_back(objective_text{
          .marker{objectives.integer<uint8_t>()},
          .text{counted_text(objectives)},
        });
      }
      if(static_cast<text_opcode>(objectives.integer<uint8_t>()) != text_opcode::end) objectives.fail("objective list lacks terminator");
      objectives.require_end();
      result.sections.push_back(mission_text_section{
        .kind{mission_text_kind::objectives},
        .content{std::move(entries)},
      });
    } else file.fail("unknown mission text section");
  }
  return result;
}

auto render_text(text_program const &source, std::span<int16_t const> counters)->std::string {
  /// The original interpreter selects the last matching range, preserving spaces.
  std::string result;
  for(auto const &segment : source.segments) {
    if(auto const *literal{std::get_if<std::string>(&segment)}) result += *literal;
    else {
      auto const &phrase{std::get<counter_phrase>(segment)};
      if(phrase.counter >= counters.size()) throw std::runtime_error{"mission text refers to an unavailable counter"};
      auto const value{counters[phrase.counter]};
      conditional_phrase const *selected{};
      for(auto const &choice : phrase.choices) {
        if(choice.first <= value && value <= choice.last) selected = &choice;
      }
      if(selected) result += selected->text;
    }
  }
  return result;
}

auto mission_text::find(mission_text_kind kind, unsigned int index) const->mission_text_section const * {
  for(auto const &section : sections) {
    if(section.kind != kind) continue;
    if(!index) return &section;
    --index;
  }
  return nullptr;
}

} // namespace earthsiege
