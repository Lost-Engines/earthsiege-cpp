#include "paths.h"
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace earthsiege {

auto executable_path(char const *argument)->std::filesystem::path {
#ifdef _WIN32
  std::vector<wchar_t> buffer(32768);
  auto const size{GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()))};
  if(size && size < buffer.size()) return std::filesystem::path{std::wstring{buffer.data(), size}};
#elif defined(__APPLE__)
  uint32_t size{};
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buffer(size);
  if(_NSGetExecutablePath(buffer.data(), &size) == 0) return std::filesystem::weakly_canonical(buffer.data());
#elif defined(__linux__)
  std::error_code error;
  auto const path{std::filesystem::read_symlink("/proc/self/exe", error)};
  if(!error) return path;
#endif
  return std::filesystem::absolute(argument);
}
auto user_save_directory()->std::filesystem::path {
  std::filesystem::path base;
#ifdef _WIN32
  if(auto const *value{_wgetenv(L"APPDATA")}; value && *value) base = value;
#else
  if(auto const *home{std::getenv("HOME")}; home && *home) {
#ifdef __APPLE__
    base = std::filesystem::path{home} / "Library/Application Support";
#else
    base = std::filesystem::path{home} / ".local/share";
#endif
  }
#ifndef __APPLE__
  if(auto const *xdg{std::getenv("XDG_DATA_HOME")}; xdg && std::filesystem::path{xdg}.is_absolute()) base = xdg;
#endif
#endif
  if(base.empty()) throw std::runtime_error{"cannot locate user data directory; use --save-dir"};
  auto const result{base / "lost-engines/earthsiege"};
  std::filesystem::create_directories(result);
  return result;
}

} // namespace earthsiege
