#include "input_tape.h"

namespace earthsiege {

auto read_input_tape(bytes source)->input_tape {
  /// ES1 DBSIM 0x4cb14: 14-byte input record, mouse count, then mouse events.
  binary_reader file{source, "recorded input tape"};
  input_tape result;
  for(auto &starting_file : result.starting_files) {
    auto const data{file.take(file.integer<uint32_t>())};
    starting_file.assign(data.begin(), data.end());
  }
  result.stream_offset = file.position();
  while(file.remaining()) {
    tape_frame frame{
      .command{file.integer<uint16_t>()},
      .primary_axes{file.integer<int16_t>(), file.integer<int16_t>()},
      .secondary_axes{file.integer<int16_t>(), file.integer<int16_t>()},
      .delta{file.integer<uint16_t>()},
      .buttons{file.integer<uint16_t>()},
      .mouse{},
    };
    auto const count{file.integer<uint32_t>()};
    if(count > file.remaining() / 14) file.fail("mouse events exceed tape frame extent");
    frame.mouse.reserve(count);
    for(unsigned int index{}; index < count; ++index) {
      frame.mouse.push_back(tape_mouse_event{
        .position{file.integer<int32_t>(), file.integer<int32_t>()},
        .kind{file.integer<uint16_t>()},
        .timestamp{file.integer<uint32_t>()},
      });
    }
    result.frames.push_back(std::move(frame));
  }
  return result;
}

} // namespace earthsiege
