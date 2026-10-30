#pragma once
#include "shell/preferences.h"
namespace earthsiege {
enum class mission_preference_action {none,controls,done};
/// Original DBSIM 0x47a88 PRF_ALRT policy. Disk writes affect only slots 0–6.
class mission_preferences {
 preferences &prefs;
 preferences::values persisted_;
 bool radio_available;
public:
 mission_preferences(preferences &p,bool radio=true):prefs{p},persisted_{p.current()},radio_available{radio}{}
 mission_preference_action activate(unsigned index,bool reverse=false);
 preferences::values const &persisted()const{return persisted_;}
};
}
