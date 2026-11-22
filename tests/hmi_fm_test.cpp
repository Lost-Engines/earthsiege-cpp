#include <cstdlib>
#include <format>
#include <iostream>
#include "audio/hmi_fm.h"
int main(int argc, char **argv) try {
  using earthsiege::binary_reader;
  using earthsiege::hmi_fm;
  using earthsiege::opl_write;
  using earthsiege::read_file;
  if(argc != 2) return EXIT_FAILURE;
  auto const raw{read_file(argv[1])};
  binary_reader f{raw};
  auto const magic{f.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFMDRV1") f.fail("fixture version");
  auto const melodic{f.take(f.integer<uint32_t>())};
  auto const drums{f.take(f.integer<uint32_t>())};
  size_t commands{0};
  size_t writes{0};
  for(bool const stereo : {false, true}) {
    hmi_fm fm{stereo, melodic, drums};
    auto const count{f.integer<uint32_t>()};
    for(uint32_t i{0}; i <= count; ++i) {
      if(i) {
        fm.midi(f.take(f.integer<uint8_t>()));
        ++commands;
      }
      std::vector<opl_write> expected(f.integer<uint32_t>());
      for(auto &v : expected) {
        v.reg = f.integer<uint16_t>();
        v.value = f.integer<uint8_t>();
      }
      auto const actual{fm.take_writes()};
      writes += actual.size();
      if(actual != expected) {
        size_t j{0};
        while(j < actual.size() && j < expected.size() && actual[j] == expected[j]) ++j;
        throw std::runtime_error{std::format("OPL{} command {} write {}: actual {:x}:{:x} expected {:x}:{:x}; counts {} / {}", stereo ? 3 : 2, i, j, j < actual.size() ? actual[j].reg : 0, j < actual.size() ? actual[j].value : 0, j < expected.size() ? expected[j].reg : 0, j < expected.size() ? expected[j].value : 0, actual.size(), expected.size())};
      }
      auto check{[&](auto const &values){
        for(auto const value : values) {
          auto const original{f.integer<uint32_t>()};
          if(value != original) throw std::runtime_error{std::format("OPL{} command {} state {} / {}", stereo ? 3 : 2, i, value, original)};
        }
      }};
      auto const &s{fm.state()};
      check(s.program);
      check(s.channel);
      check(s.bend);
      check(s.bent);
      check(s.range);
      check(s.volume);
      check(s.velocity);
      check(s.sustain);
      if(stereo) check(s.pan);
      for(auto const &queue : s.deferred) {
        if(queue.size() != f.integer<uint32_t>()) throw std::runtime_error{"sustain queue"};
      }
      for(auto const note : s.note) {
        if(note != f.integer<uint8_t>()) throw std::runtime_error{"voice note"};
      }
    }
  }
  f.require_end();
  std::cout << commands << " original FM MIDI commands and " << writes << " register writes match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
