#pragma once
#include "audio/music_settings.h"
#include <optional>
namespace earthsiege {
std::optional<music_device> choose_music_device(music_device current);
}
