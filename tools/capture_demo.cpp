#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "shell/demo_playback.h"

using namespace earthsiege;

int main(int argc, char **argv) try {
  if(argc != 6) throw std::runtime_error{"Usage: capture_demo ASSETS DEMO PREFS.CFG MUSIC_DEVICE SCRATCH_DIR > frames.pal8"};
#ifdef _WIN32
  if(_setmode(_fileno(stdout), _O_BINARY) == -1) throw std::runtime_error{"Cannot open binary video output"};
#endif
  std::ios::sync_with_stdio(false);
  std::cout.exceptions(std::ios::badbit | std::ios::failbit);
  resource_store resources{argv[1]};
  preferences const prefs{read_file(argv[3]), true, resources.edition()};
  music_settings music;
  music.device = parse_music_device(argv[4]);
  std::filesystem::path const scratch{argv[5]};
  std::filesystem::create_directories(scratch);
  demo_playback replay{resources, static_cast<unsigned>(std::stoul(argv[2])), prefs, scratch / "saves", music};
  std::ofstream audio{scratch / "stereo.s16le", std::ios::binary};
  audio.exceptions(std::ios::badbit | std::ios::failbit);
  uint64_t time_ms{0}, video_frames{0};
  while(!replay.done()) {
    replay.step([&](unsigned hold_ms, framebuffer const &page, framebuffer const *erased) {
      auto pcm{replay.view().take_audio()};
      if constexpr(std::endian::native == std::endian::big) {
        for(auto &sample : pcm) sample = std::byteswap(sample);
      }
      audio.write(reinterpret_cast<char const *>(pcm.data()), static_cast<std::streamsize>(pcm.size() * sizeof(int16_t)));

      std::array<uint32_t, 256> palette;
      std::ranges::transform(replay.view().colours().colours, palette.begin(), [](uint32_t rgb){return rgb | 0xff000000u;});
      auto const emit_until{[&](framebuffer const &surface, uint64_t end_ms) {
        // FFmpeg PAL8 carries a fresh native-endian ARGB palette after every raster.
        while(video_frames * 1000 < end_ms * vga_refresh_hz) {
          auto const &pixels{surface.pixels()};
          std::cout.write(reinterpret_cast<char const *>(pixels.data()), pixels.size());
          std::cout.write(reinterpret_cast<char const *>(palette.data()), sizeof(palette));
          ++video_frames;
        }
      }};
      auto const end{time_ms + hold_ms};
      auto const pans{replay.view().take_pan_frames()};
      for(size_t i{0}; i < pans.size(); ++i) {
        emit_until(pans[i], std::min(end, time_ms + (i + 1) * uint64_t{1000} / vga_refresh_hz));
      }
      if(erased) {
        emit_until(page, end - std::min(hold_ms, 1000 / vga_refresh_hz));
        emit_until(*erased, end);
      } else {
        emit_until(page, end);
      }
      time_ms = end;
      return true;
    });
  }
  if(replay.packets_read() != replay.packet_count()) throw std::runtime_error{"Demo ended before the tape was consumed"};
  std::cout.flush();
  audio.close();
  std::ofstream summary{scratch / "capture.txt"};
  summary.exceptions(std::ios::badbit | std::ios::failbit);
  summary << time_ms << ' ' << replay.packets_read() << ' ' << video_frames << '\n';
  summary.close();
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
