#pragma once

#include <optional>
#include "resources/edition.h"
#include <string_view>
#include "audio/music_settings.h"

namespace earthsiege {
class window;
class resource_store;
auto choose_game_edition(window &display, resource_store const &resources)->std::optional<game_edition>;
auto choose_music_device(window &display, resource_store const &resources, music_device current)->std::optional<music_device>;
void show_music_error(window &display, resource_store const &resources, std::string_view message);
} // namespace earthsiege
