#pragma once
#include "shell/preferences.h"
namespace earthsiege {
enum class control_setting_action { none, calibrate, done };
/// GO 0x64c4 option policy; physical device creation is supplied by the host.
class control_settings_state {
 preferences &prefs;
 std::array<uint8_t,10> state_{};
 preferences::values persisted_;
 void apply();
public:
 explicit control_settings_state(preferences &);
 control_setting_action activate(unsigned button,bool reverse=false,bool keyboard=false);
 std::array<uint8_t,10> const &state()const{return state_;}
 preferences::values const &persisted()const{return persisted_;}
};
}
