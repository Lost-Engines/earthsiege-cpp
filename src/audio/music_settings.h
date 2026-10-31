#pragma once
#include <filesystem>
#include <string_view>
#include <vector>
#include <cstdint>
namespace earthsiege {
enum class music_device { opl2, opl3, mt32, mt32_mpu, sc55, scc1a, soundfont };
struct music_settings {
  music_device device{music_device::opl3};
  std::filesystem::path rom_directory, soundfont, program_directory;
};
std::string_view music_device_name(music_device);
std::string_view music_device_label(music_device);
music_device parse_music_device(std::string_view);
uint32_t music_driver_id(music_device);
std::filesystem::path music_asset_directory(std::filesystem::path const &program_directory={});
std::vector<std::filesystem::path> music_asset_directories(music_settings const &,std::filesystem::path const &data);
music_settings read_music_settings(std::filesystem::path const &file);
void write_music_settings(std::filesystem::path const &file,music_settings const &);
}
