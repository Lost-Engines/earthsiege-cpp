#include "resources/cockpit.h"
#include "resources/combat.h"
#include "resources/ground_definition.h"
#include "resources/herc_definition.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include "resources/player_roster.h"
#include "resources/projectile_motion.h"
#include "resources/resource_store.h"
#include "resources/world.h"
#include "shell/preferences.h"
#include "simulation/scenery_world.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool v, std::string const &context) {
  if (!v)
    throw std::runtime_error{context};
}
template <class T> void eq(binary_reader &f, T actual, std::string const &label) {
  require(actual == f.integer<T>(), label + " differs");
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 3)
    return EXIT_FAILURE;
  resource_store resources{argv[1]};
  require(resources.edition() == game_edition::expansion, "edition detection");
  auto fixture{read_file(argv[2])};
  binary_reader f{fixture, "expansion original captures"};
  auto magic{f.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESEXP001", "fixture version");
  auto lookups{f.integer<uint32_t>()};
  for (uint32_t i{}; i < lookups; ++i) {
    auto mask{f.integer<uint16_t>()};
    auto name{f.cstring()};
    eq(f, crc32(resources.read(mask == 1 ? "simvol0" : "shell0", name)), name + " priority");
  }
  auto worlds{f.integer<uint16_t>()};
  for (unsigned i{}; i < worlds; ++i) {
    auto name{f.cstring()};
    auto w{read_world_environment(resources.read("simvol0", "wld/" + name + ".WLD"), resources.edition())};
    auto const &b{w.background};
    for (auto v : {b.horizon_band_width, b.sky_palette_start, b.sky_step, b.sky_bands, b.ground_palette_start,
                   b.ground_step, b.ground_bands, b.horizon_offset})
      eq(f, v, name);
    for (auto v : w.atmosphere_parameters)
      eq(f, v, name);
    for (auto v : w.expansion_parameters)
      eq(f, v, name);
    for (auto const &band : w.distance_bands) {
      eq(f, static_cast<uint32_t>(band.size()), name);
      for (auto v : band)
        eq(f, v, name);
    }
    eq(f, w.shade_levels, name);
    auto ranges = [&](auto const &rows) {
      eq(f, static_cast<uint16_t>(rows.size()), name);
      for (auto r : rows) {
        eq(f, r.first, name);
        eq(f, r.count, name);
      }
    };
    ranges(w.near_ranges);
    ranges(w.far_ranges);
    ranges(w.blend_ranges);
    eq(f, w.stored_far_range_count, name);
    for (auto v : w.effect_parameters)
      eq(f, v, name);
    for (auto v : w.model_distance_offsets)
      eq(f, v, name);
    for (auto const &v : w.resource_names)
      require(v == f.cstring(), name + " resource binding");
    mission_header h{.terrain{static_cast<terrain_kind>(i / 6)},
                     .zone{-1},
                     .world_variant{static_cast<int16_t>(i % 2)},
                     .palette_variant{static_cast<int16_t>(i % 6 / 2)}};
    scenery_world owned{resources, h};
    require(owned.environment().resource_names == w.resource_names, name + " owned world");
  }
  auto weapons{read_weapon_catalogue(resources)};
  eq(f, static_cast<uint16_t>(weapons.weapons.size()), "weapons");
  for (auto const &w : weapons.weapons) {
    for (auto v : w.model_indices)
      eq(f, v, "weapon model");
    eq(f, w.internal_maximum, "internal capacity");
    eq(f, w.minimum_range, "minimum range");
    eq(f, w.range, "range");
    eq(f, w.shot_value_penalty, "penalty");
    for (auto v : w.energy_thresholds)
      eq(f, v, "charge threshold");
    eq(f, w.magazine, "magazine");
    eq(f, w.barrels, "barrels");
    eq(f, w.projectile, "projectile");
    for (int i{}; i < 3; ++i)
      eq(f, w.muzzle_offset[i], "muzzle");
    for (int i{}; i < 3; ++i)
      eq(f, w.side_muzzle_offset[i], "side muzzle");
    eq(f, w.refire_delay, "refire");
    eq(f, w.combat_rating, "rating");
    eq(f, w.damage_icon, "damage icon");
  }
  eq(f, static_cast<uint16_t>(weapons.projectiles.size()), "projectiles");
  for (auto const &p : weapons.projectiles) {
    eq(f, static_cast<int16_t>(p.kind), "projectile kind");
    eq(f, p.subtype, "subtype");
    eq(f, p.shield_damage, "shield damage");
    eq(f, p.armour_damage, "armour damage");
    eq(f, p.splash_q10, "splash");
    eq(f, p.speed, "speed");
    for (auto v : p.shield_effects)
      eq(f, v, "shield effect");
    for (auto v : p.ground_effects)
      eq(f, v, "ground effect");
    for (auto v : p.armour_effects)
      eq(f, v, "armour effect");
  }
  auto bullets{read_bullet_motions(resources.read("simvol0", "dat/BULLETS.DAT"), resources.edition())};
  eq(f, static_cast<uint16_t>(bullets.size()), "bullets");
  for (auto b : bullets)
    for (auto v :
         {b.model, b.lifetime, b.collision_radius, b.animation_period, b.shot_noise, b.spread, b.damage_falloff})
      eq(f, v, "bullet motion");
  auto cockpits{f.integer<uint16_t>()};
  for (unsigned i{}; i < cockpits; ++i) {
    auto name{f.cstring()};
    auto chassis{f.integer<uint16_t>()};
    auto c{read_cockpit(resources, name)};
    require(c.panels[0].width == 320, "cockpit bitmap");
    auto world{read_palette(resources.read("simvol0", "dpl/WORLD0.DPL"))};
    auto bank{read_palette(resources.read("simvol0", "dpl/COCKPIT.DPL"))};
    apply_cockpit_colours(world, bank, chassis, resources.edition());
    for (auto rgb : world.dac_colours)
      for (auto v : rgb)
        eq(f, v, name + " native palette");
  }
  auto tapes{f.integer<uint16_t>()};
  size_t frame_count{};
  for (unsigned i{}; i < tapes; ++i) {
    auto name{f.cstring()};
    auto raw{read_file(resources.root() / "TAPES" / name)};
    auto tape{read_input_tape(raw, resources.edition())};
    eq(f, static_cast<uint32_t>(tape.frames.size()), name + " frame count");
    for (auto const &frame : tape.frames) {
      eq(f, frame.command, name);
      for (int j{}; j < 2; ++j)
        eq(f, frame.primary_axes[j], name);
      for (int j{}; j < 2; ++j)
        eq(f, frame.secondary_axes[j], name);
      eq(f, frame.delta, name);
      eq(f, frame.buttons, name);
      eq(f, frame.extended_buttons, name);
      eq(f, static_cast<uint32_t>(frame.mouse.size()), name);
      eq(f, static_cast<uint32_t>(frame.commands.size()), name);
      for (auto m : frame.mouse) {
        for (int j{}; j < 2; ++j)
          eq(f, m.position[j], name);
        eq(f, m.kind, name);
        eq(f, m.timestamp, name);
      }
      for (auto v : frame.commands)
        eq(f, v, name);
      ++frame_count;
    }
    auto const &input{tape.starting_files[std::to_underlying(tape_file::preferences)]};
    preferences p{input, false, resources.edition()};
    require(p.current().size() == 22, "expanded preferences");
    for (size_t j{}; j < 22; ++j)
      require(p.current()[j] == (j < input.size() ? input[j] : 0), "zero-filled short preference file");
  }
  auto handoffs{f.integer<uint16_t>()};
  for (unsigned i{}; i < handoffs; ++i) {
    auto name{f.cstring()};
    auto raw{f.take(f.integer<uint32_t>())};
    auto player{read_player_roster(raw, resources.edition())};
    require(crc32(write_player_roster(player)) == crc32(raw), name + " roster roundtrip");
    auto script{f.take(f.integer<uint32_t>())};
    require(crc32(write_mission_script(read_mission_script(script))) == crc32(script), name + " mission roundtrip");
    f.take(f.integer<uint32_t>());
  }
  f.require_end();
  auto names{read_resource_names(resources.read("simvol0", "nam/MECHS.NAM"))};
  require(names.size() == 19, "expansion HERC roster");
  for (auto const &n : names) {
    auto h{read_herc_definition(resources, n)};
    require(!h.models.roots.empty() && !h.textures.empty(), n + " geometry and texture binding");
  }
  ground_catalogue buildings{resources};
  require(buildings.profiles().size() == 11, "expansion building count");
  require(buildings.profile(10).type == 10 && buildings.profile(10).collision_enabled, "landing craft definition");
  require(worlds == 36 && frame_count == 11421 && handoffs == 74, "reference coverage");
  std::cout << lookups << " priority lookups, " << worlds
            << " worlds, 22 weapons, 23 projectiles, 19 HERCs, 11 buildings, " << cockpits << " cockpits, "
            << frame_count << " native input packets and " << handoffs << " original mission handoffs agree\n";
  return EXIT_SUCCESS;
} catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
