#pragma once

#include <optional>
#include <string_view>
#include "audio/music_settings.h"

namespace earthsiege {
class window;
class resource_store;
auto choose_music_device(window &display, resource_store const &resources, music_device current)->std::optional<music_device>;
void show_music_error(window &display, resource_store const &resources, std::string_view message);
} // namespace earthsiege
