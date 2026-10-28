#pragma once
#include <array>
#include <functional>
#include "simulation/weapon_bank.h"
#include "simulation/weapon_display.h"

namespace earthsiege {

/// The player's three editable weapon groups and their original selection policy.
/// Owns control state; the HERC continues to own ammunition, capacitors and mounts.
class player_armament {
  weapon_bank &bank;
  weapon_catalogue const &definitions;
  std::span<weapon_mount const> mounts;
  std::array<std::vector<bool>, 3> groups;
  std::vector<bool> pairs;
  int16_t selected{-1};
  uint8_t group{0};
  bool held_selection{false};
  bool tracking{false};
  bool manual_fired{false};
  bool manual_active{false};
  std::vector<weapon_display> instrument_rows;
  std::optional<uint8_t> requested_group;
  std::optional<bool> requested_tracking;
  bool requested_link{false};
  bool keyboard_group_changed{false};
  bool available(size_t slot) const;
  bool ready(size_t slot) const;
  bool poll_trigger(size_t slot, bool pressed, bool &release) const;

public:
  player_armament(weapon_bank &weapons, weapon_catalogue const &catalogue, std::span<weapon_mount const> attachment_points);
  void initialise_displays();
  std::span<weapon_display const> displays() const;
  void request_group_control(unsigned int button);
  void exchange_group_controls();
  bool click_control(unsigned int control_number, bool secondary);
  void request_charge(unsigned int control_number, int32_t value);
  void exchange_displays(int32_t distance, std::array<bool, 5> const &locks, frame_ratio ratio);
  int16_t selection() const;
  uint8_t current_group() const;
  bool included(size_t group_index, size_t slot) const;
  bool linked(size_t slot) const;
  bool holds_selection() const;
  bool auto_tracking() const;
  bool manual_launched() const;
  bool manual_guidance() const;
  bool select(size_t slot);
  void cycle(int16_t direction = 1);
  void change_group(uint8_t index);
  void cycle_group();
  void toggle_inclusion(size_t slot);
  bool toggle_category(int8_t category);
  void toggle_pair();
  void refresh_pairs();
  void default_charge();
  void fast_charge();
  void adjust_charge(bool increase);
  void set_tracking(bool enabled);
  void hold_selection(bool held);
  void set_manual_guidance(bool active);
  void finish_manual_guidance();
  /// Range and seeker locks affect automatic selection, not the trigger gate.
  bool selectable(size_t slot, int32_t distance, std::array<bool, 5> const &locks) const;
  void update_selection(int32_t distance, std::array<bool, 5> const &locks);
  /// Both linked weapons must be ready before either trigger method executes.
  /// The callback discharges the actual weapon and updates its readiness.
  /// Returns the original grenade trigger's request to release the input button.
  bool fire(bool pressed, std::function<void(size_t)> const &discharge);
  /// DOS application command codes, offered here before HERC/cockpit fallbacks.
  bool command(uint16_t code);
};

} // namespace earthsiege
