#pragma once
#include "music_settings.h"
#include "midi_receiver.h"
#include <memory>
namespace earthsiege {
std::unique_ptr<midi_receiver> make_music_synth(music_settings const &,std::filesystem::path const &data,unsigned rate=44100);
}
