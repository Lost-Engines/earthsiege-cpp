#include "mission_script.h"
#include <array>
#include <bit>
#include "resources/mission_records.h"

namespace earthsiege {
namespace {

template<size_t Count>
auto words(binary_reader &file)->std::array<int16_t, Count> {
  std::array<int16_t, Count> result{};
  for(auto &value : result) value = file.integer<int16_t>();
  return result;
}

template<typename F>
auto table(binary_reader &file, F read) {
  using T = decltype(read());
  auto const count{file.integer<uint16_t>()};
  if(count > 1000) file.fail("excessive compiled mission table count");
  std::vector<T> result;
  result.reserve(count);
  for(unsigned int index{}; index < count; ++index) result.push_back(read());
  return result;
}

} // namespace

auto read_routes_record(binary_reader &file)->mission_route {
  mission_route route;
  auto const count{file.integer<uint16_t>()};
  route.points.reserve(count);
  for(unsigned int index{}; index < count; ++index) route.points.push_back(file.integer<int16_t>());
  return route;
}

auto read_areas_record(binary_reader &file)->trigger_area {
  return trigger_area{
    .kind{file.integer<int16_t>()},
    .first_point{file.integer<int16_t>()},
    .second_point_or_radius{file.integer<int16_t>()},
  };
}

auto read_triggers_record(binary_reader &file)->mission_trigger {
  return mission_trigger{
    .mode{file.integer<int16_t>()},
    .test{file.integer<int16_t>()},
    .areas{words<8>(file)},
    .delay{file.integer<int16_t>()},
    .argument{file.integer<int16_t>()},
  };
}

auto read_hercs_record(binary_reader &file)->herc_spawn {
  return herc_spawn{
    .field_mask{words<20>(file)},
    .chassis{file.integer<int16_t>()},
    .weapons{words<10>(file)},
    .position{file.integer<int16_t>()},
    .heading{file.integer<int16_t>()},
    .counter{file.integer<int16_t>()},
    .counter_operation{file.integer<int16_t>()},
    .ammunition{words<10>(file)},
    .skill{file.integer<int16_t>()},
    .trigger_a{file.integer<int16_t>()},
    .trigger_b{file.integer<int16_t>()},
    .initial_condition{file.integer<int16_t>()},
  };
}

auto read_flyers_record(binary_reader &file)->flyer_spawn {
  return flyer_spawn{
    .field_mask{words<20>(file)},
    .position{file.integer<int16_t>()},
    .heading{file.integer<int16_t>()},
    .type{file.integer<int16_t>()},
    .counter{file.integer<int16_t>()},
    .counter_operation{file.integer<int16_t>()},
    .trigger_a{file.integer<int16_t>()},
    .trigger_b{file.integer<int16_t>()},
    .initial_value{file.integer<int16_t>()},
  };
}

auto read_bases_record(binary_reader &file)->base_spawn {
  return base_spawn{
    .type{file.integer<int16_t>()},
    .position{file.integer<int16_t>()},
    .heading{file.integer<int16_t>()},
    .counter{file.integer<int16_t>()},
    .counter_operation{file.integer<int16_t>()},
    .trigger_a{file.integer<int16_t>()},
    .trigger_b{file.integer<int16_t>()},
    .initial_value{file.integer<int16_t>()},
  };
}

auto read_groups_record(binary_reader &file)->group_spawn {
  return group_spawn{
    .field_mask{words<20>(file)},
    .kind{static_cast<actor_kind>(file.integer<int16_t>())},
    .formation{file.integer<int16_t>()},
    .argument{file.integer<int16_t>()},
    .position{file.integer<int16_t>()},
    .heading{file.integer<int16_t>()},
    .route{file.integer<int16_t>()},
    .members{words<8>(file)},
    .mode{file.integer<int16_t>()},
    .target_type{static_cast<target_kind>(file.integer<int16_t>())},
    .target{file.integer<int16_t>()},
    .trigger{file.integer<int16_t>()},
    .extra{words<3>(file)},
  };
}

auto read_mission_script(bytes source)->mission_script {
  /// GO 0x3445b -> DBSIM 0x1d850: nine reference/roster tables after ten settings.
  binary_reader file{source, "compiled mission"};
  mission_script result;
  result.header = mission_header{
    .terrain{static_cast<terrain_kind>(file.integer<int16_t>())},
    .zone{file.integer<int16_t>()},
    .difficulty{file.integer<int16_t>()},
    .settings{words<5>(file)},
    .world_variant{file.integer<int16_t>()},
    .palette_variant{file.integer<int16_t>()},
  };
  result.points = table(file, [&file]{ return vec3<world_coordinate>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; });
  result.headings = table(file, [&file]{ return file.integer<int16_t>(); });
  result.routes = table(file, [&file]{ return read_routes_record(file); });
  result.areas = table(file, [&file]{ return read_areas_record(file); });
  result.triggers = table(file, [&file]{ return read_triggers_record(file); });
  result.hercs = table(file, [&file]{ return read_hercs_record(file); });
  result.flyers = table(file, [&file]{ return read_flyers_record(file); });
  result.bases = table(file, [&file]{ return read_bases_record(file); });
  result.groups = table(file, [&file]{ return read_groups_record(file); });
  result.consumed = file.position();                                          // the original writer leaves stale tails in existing files
  return result;
}

auto decode_mission_heading(int16_t stored)->native_angle {
  /// DBSIM 0x1dd62 shifts the stored value by fourteen, retaining the low word.
  return static_cast<uint16_t>(static_cast<uint32_t>(std::bit_cast<uint16_t>(stored)) << 14);
}

} // namespace earthsiege
