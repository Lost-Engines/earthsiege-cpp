#pragma once
#include "resources/binary_reader.h"
#include <array>
#include <vector>

namespace earthsiege {
struct opl_write {uint16_t reg{};uint8_t value{};bool operator==(opl_write const&)const=default;};
struct fm_driver_state {
  std::array<uint32_t,16> program{},bend{},bent{},range{},volume{},sustain{},pan{};
  std::array<uint32_t,9> channel{},velocity{};
  std::array<uint8_t,9> note{};
  std::array<std::vector<std::array<uint8_t,3>>,16> deferred;
};
// Register-level reconstruction of the supplied HMI FM MIDI modules. The OPL3
// module uses nine paired stereo voices, rather than eighteen independent ones.
class hmi_fm {
  bool stereo;
  fm_driver_state state_;
  struct bank {std::vector<uint8_t> raw;uint32_t names{},patches{};};
  bank melodic,drums;
  std::array<uint8_t,32> level{};
  // The original release cache has only sixteen bytes: operator offsets 16+
  // alias the nine low-frequency bytes followed by nine high-frequency bytes.
  std::array<uint8_t,34> release{};
  std::vector<opl_write> writes;
  static auto read_bank(bytes source)->bank;
  void write(unsigned int reg,unsigned int value);
  void both(unsigned int reg,unsigned int value);
  auto patch(bool percussion,unsigned int id)const->bytes;
  auto allocate(unsigned int channel)const->unsigned int;
  void voice_off(unsigned int voice);
  void set_patch(unsigned int voice,bytes patch);
  void frequency(unsigned int voice,uint32_t value,bool start);
  auto bent_frequency(unsigned int voice)const->uint32_t;
  auto attenuation(unsigned int voice,unsigned int amplitude)const->uint8_t;
  auto amplitude(unsigned int voice)const->unsigned int;
  void note_on(uint8_t channel,uint8_t note,uint8_t velocity);
  void note_off(uint8_t channel,uint8_t note,uint8_t velocity);
  void volume(uint8_t channel,uint8_t value);
  void pan(uint8_t channel,uint8_t value);
  void reset(uint8_t channel,bool all_notes);
public:
  hmi_fm(bool opl3,bytes melodic_bank,bytes drum_bank);
  void midi(bytes message);
  auto take_writes()->std::vector<opl_write>;
  auto state()const->fm_driver_state const& {return state_;}
};
}
