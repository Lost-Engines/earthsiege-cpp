#pragma once

#include <filesystem>

namespace earthsiege {

auto executable_path(char const *argument)->std::filesystem::path;
auto user_save_directory()->std::filesystem::path;

} // namespace earthsiege
