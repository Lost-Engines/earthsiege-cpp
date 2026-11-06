#pragma once
#include <filesystem>
#include <optional>
#include "resources/career_index.h"
#include "shell/new_career.h"
#include "shell/preferences.h"
namespace earthsiege {
/// Original save formats in a separate user-writable directory. Archive data
/// and a user's comparison DOS installation are never modified.
class save_store {
  std::filesystem::path directory;
  game_edition edition_;
  int16_t battle_set_{};
  std::filesystem::path path(std::string_view name) const;
public:
  explicit save_store(std::filesystem::path root,game_edition edition=game_edition::original):directory{std::move(root)},edition_{edition}{}
  void battle_set(int16_t value){battle_set_=value;}
  std::filesystem::path const &root()const{return directory;}
  std::optional<std::vector<uint8_t>> read(std::string_view name)const;
  void write(std::string_view name,bytes data)const;
  career_index index()const;
  void save_index(career_index const &index)const;
  campaign_start load(career_entry const &entry)const;
  void save(career_entry const &entry,campaign_start const &career)const;
  void erase(career_entry const &entry)const;
  preferences load_preferences()const;
  void save_preferences(preferences const &settings)const;
};
}
