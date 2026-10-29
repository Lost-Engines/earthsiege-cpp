#pragma once
#include "resources/career.h"
namespace earthsiege {
/// Original warning dialog ID, or zero when the action can continue.
/// Squad scans retain the original crew-cursor and selected-HERC side effects.
unsigned launch_warning(hangar &roster,career_state &crew);
unsigned next_mission_warning(hangar &roster,career_state &crew);
bool construction_available(hangar const &roster);
}
