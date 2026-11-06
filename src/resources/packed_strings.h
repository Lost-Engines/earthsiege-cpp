#pragma once
#include "resources/edition.h"
#include "resources/binary_reader.h"
namespace earthsiege {
/// GO's offset table and CP437 string pool, shared by dialog and shell text.
std::vector<std::string> read_packed_strings(bytes source);
/// Logical shell labels retain their base IDs; expansion-only names have
/// explicit accessors because inserted strings change the native table offsets.
class shell_text {
 std::vector<std::string> entries;
 game_edition edition;
public:
 shell_text(bytes source,game_edition version):entries{read_packed_strings(source)},edition{version}{}
 std::string const &at(size_t original_id)const;
 std::string const &native(size_t id)const{return entries.at(id);}
 std::string const &chassis(int16_t id)const;
 std::string const &weapon(int16_t id,bool campaign)const;
 std::string const &weapon_condition(int16_t id)const;
 std::string const &weapon_full(int16_t id)const;
};
}
