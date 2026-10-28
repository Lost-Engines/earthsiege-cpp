#include "presentation.h"
#include <optional>
#include <stdexcept>

namespace earthsiege {
namespace {
void read_chunks(bytes source, presentation_program &program, std::optional<uint16_t> &pages, unsigned int depth) {
  binary_reader file{source, "TTM chunks"};
  if(depth > 16) file.fail("excessive container nesting");
  while(file.remaining()) {
    auto const tag_bytes{file.take(4)}; std::string const tag{tag_bytes.begin(), tag_bytes.end()};
    auto const size{file.integer<uint32_t>()}; auto const payload{file.take(size & 0x7fffffff)};
    if(size & 0x80000000) { read_chunks(payload, program, pages, depth + 1); continue; }
    binary_reader body{payload, tag};
    if(tag == "PAG:") { pages = body.integer<uint16_t>(); body.require_end(); }
    else if(tag == "TAG:") {
      auto const count{body.integer<uint16_t>()};
      for(unsigned int i{}; i < count; ++i) program.labels.push_back({body.integer<uint16_t>(), body.cstring()});
      body.require_end();
    } else if(tag == "TT3:") {
      if(!program.bytecode.empty()) body.fail("multiple bytecode sections");
      if(body.integer<uint8_t>() != 1) body.fail("unsupported bytecode compression");
      auto const amount{body.integer<uint32_t>()};
      if(amount > 16 * 1024 * 1024) body.fail("excessive bytecode size");
      program.bytecode.reserve(amount);
      while(program.bytecode.size() < amount) {
        auto const code{body.integer<uint8_t>()}; size_t const count{static_cast<size_t>(code & 127)};
        if(count > amount - program.bytecode.size()) body.fail("RLE output overrun");
        if(code & 128) program.bytecode.insert(program.bytecode.end(), count, body.integer<uint8_t>());
        else { auto const run{body.take(count)}; program.bytecode.insert(program.bytecode.end(), run.begin(), run.end()); }
      }
      body.require_end();
    }
  }
}
} // namespace

uint8_t presentation_program::label_page(int16_t const number) const {
  for(size_t i{}; i < pages.size(); ++i) {
    auto const &page{pages[i]};
    if(!page.empty() && (page.front().opcode == 0x1101 || page.front().opcode == 0x1111) && page.front().arguments.front() == number)
      return static_cast<uint8_t>(i);
  }
  return 0;
}
presentation_program read_presentation(bytes source) {
  presentation_program result; std::optional<uint16_t> page_count;
  read_chunks(source, result, page_count, 0);
  binary_reader code{result.bytecode, "TTM program"};
  std::vector<presentation_instruction> page;
  while(code.remaining()) {
    presentation_instruction instruction{code.integer<uint16_t>(), {}, {}, {}};
    auto const arity{instruction.opcode & 15};
    if(arity <= 7) for(int i{}; i < arity; ++i) instruction.arguments.push_back(code.integer<int16_t>());
    else if(arity == 15) {
      if(instruction.opcode == 0xaf1f || instruction.opcode == 0xaf2f) {
        auto const count{code.integer<uint16_t>()}; instruction.arguments.push_back(static_cast<int16_t>(count));
        for(unsigned int i{}; i < count * 2u; ++i) instruction.points.push_back(code.integer<int16_t>());
      } else {
        auto const start{code.position()}; instruction.text = code.cstring();
        if((code.position() - start) & 1) code.take(1);
      }
    } else code.fail("unknown operand encoding");
    bool const end{instruction.opcode == 0xff0};
    page.push_back(std::move(instruction));
    if(end) { result.pages.push_back(std::move(page)); page.clear(); }
  }
  if(!page_count || *page_count > 255 || result.pages.size() != *page_count || !page.empty()) code.fail("page count mismatch");
  return result;
}
} // namespace earthsiege
