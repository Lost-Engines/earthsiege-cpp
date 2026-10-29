#pragma once
#include "simulation/joystick_input.h"
#include <optional>
namespace earthsiege {
struct joystick_calibration_write {unsigned slot;joystick_calibration calibration;};
/// GO 0x7631/0x7ad7/0x7ef0. Host samples replace electrical gameport reads.
class joystick_calibration_state {
 uint8_t model;bool pedals;std::array<bool,2> present;
 std::array<joystick_calibration,2> &devices;
 joystick_calibration backup_;
 int choice_{},stage_{},slot{},group{},steps{},mask{};
 bool active_{},cancelled_{},done_{};
 int message_{-1};
 void configure();
 void reset();
 std::optional<joystick_calibration_write> advance();
public:
 joystick_calibration_state(uint8_t model,bool pedals,std::array<bool,2> present,std::array<joystick_calibration,2> &);
 void activate(unsigned button);
 std::optional<joystick_calibration_write> sample(std::array<int32_t,4> const &raw);
 void cancel();
 bool active()const{return active_;}
 bool done()const{return done_;}
 unsigned selected_slot()const{return static_cast<unsigned>(slot-1);}
 int choice()const{return choice_;}
 int message()const{return message_;}
 joystick_calibration const &backup()const{return backup_;}
 std::array<int32_t,12> state()const;
};
}
