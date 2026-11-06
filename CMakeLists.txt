cmake_minimum_required(VERSION 3.30)
project(earthsiege VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)
# Portable packages include the window/audio backends and compiler runtime.
option(EARTHSIEGE_STATIC_RUNTIME "Link compiler runtimes statically" ${MINGW})
if(EARTHSIEGE_STATIC_RUNTIME)
  if(MINGW)
    add_link_options(-static)
  else()
    add_link_options("$<$<LINK_LANG_AND_ID:CXX,GNU>:-static-libgcc;-static-libstdc++>")
  endif()
endif()
include(cmake/platform_dependencies.cmake)
include(cmake/music_dependencies.cmake)

add_library(nuked_opl3 STATIC third_party/nuked-opl3/opl3.c)

add_library(earthsiege_core
  src/render/software_cursor.cpp
  src/render/palette_device.cpp
  src/shell/control_settings.cpp
  src/shell/controls_dialog.cpp
  src/shell/joystick_calibration.cpp
  src/render/control_settings_artwork.cpp
  src/render/calibration_artwork.cpp
  src/render/shell_statistics.cpp
  src/render/shell_door.cpp
  src/resources/lpf_animation.cpp
  src/render/crew_artwork.cpp
  src/render/hangar_artwork.cpp
  src/render/weapon_preview.cpp
  src/render/dialog_widgets.cpp
  src/shell/mission_alert.cpp
  src/shell/cockpit_impact.cpp
  src/shell/cockpit_startup.cpp
  src/shell/game_shell.cpp
  src/shell/launch_checks.cpp
  src/shell/shell_animation.cpp
  src/shell/bay_animation_plan.cpp
  src/shell/bay_scene.cpp
  src/render/shell_animation_artwork.cpp
  src/shell/save_store.cpp
  src/resources/career_index.cpp
  src/shell/game_state.cpp
  src/resources/packed_strings.cpp
  src/simulation/mission_return.cpp
  src/simulation/mission_salvage.cpp
  src/shell/preferences.cpp
  src/resources/mission_writer.cpp
  src/shell/mission_launch.cpp
  src/shell/new_career.cpp
  src/resources/mission_results.cpp
  src/shell/campaign_return.cpp
  src/shell/crew_assignment.cpp
  src/shell/armament.cpp
  src/shell/construction_menu.cpp
  src/render/construction_artwork.cpp
  src/shell/component_repair.cpp
  src/shell/hangar_service.cpp
  src/resources/career.cpp
  src/resources/campaign.cpp
  src/shell/tactical_controller.cpp
  src/render/full_damage_display.cpp
  src/render/tactical_map.cpp
  src/render/briefing_map.cpp
  src/audio/music_settings.cpp
  src/audio/midi_synth_factory.cpp
  src/audio/la_synth.cpp
  src/audio/sc55_synth.cpp
  src/audio/soundfont_synth.cpp
  src/audio/music_audio.cpp
  src/audio/combat_music.cpp
  src/audio/music_player.cpp
  src/audio/midi_gain.cpp
  src/audio/hmi_pcm.cpp
  src/audio/hmi_fm.cpp
  src/simulation/footsteps.cpp
  src/resources/hmp.cpp
  src/audio/hmp_player.cpp
  src/audio/audio_controls.cpp
  src/audio/mission_audio.cpp
  src/audio/shell_audio.cpp
  src/audio/pcm_mixer.cpp
  src/audio/pcm_sound.cpp
  src/audio/spatial_audio.cpp
  src/shell/cockpit_radio.cpp
  src/render/wingmate_portrait.cpp
  src/shell/pilot_transmission.cpp
  src/render/radio_portrait.cpp
  src/shell/speech_selection.cpp
  src/resources/shell_speech.cpp
  src/render/shell_caption.cpp
  src/render/shell_speech_scene.cpp
  src/render/shell_cinematic.cpp
  src/resources/portrait_sync.cpp
  src/render/radio_banner.cpp
 src/render/external_view.cpp
  src/shell/radio_controller.cpp
  src/resources/radio_catalogue.cpp
  src/simulation/radio_queue.cpp
  src/simulation/instrument_failure.cpp
  src/shell/cockpit_hdd.cpp
  src/shell/cockpit_displays.cpp
  src/simulation/mission_communications.cpp
  src/simulation/wingmate_orders.cpp
  src/simulation/mission_displays.cpp
  src/shell/mfd_controller.cpp
  src/render/missile_display.cpp
  src/render/navigation_display.cpp
  src/render/communications_display.cpp
  src/render/scanner_display.cpp
  src/render/mfd_instruments.cpp
  src/resources/paper_doll.cpp
  src/render/damage_display.cpp
  src/resources/presentation.cpp
  src/render/presentation_player.cpp
  src/render/clock_instrument.cpp
  src/render/waypoint_instruments.cpp
  src/render/target_instruments.cpp
  src/render/navigation_instruments.cpp
  src/resources/interface_strings.cpp
  src/simulation/weapon_display.cpp
  src/render/weapon_instruments.cpp
  src/shell/pointer_controls.cpp
  src/render/slider_geometry.cpp
  src/render/shield_instruments.cpp
  src/render/instrument_bars.cpp
  src/resources/cockpit.cpp
  src/simulation/player_targeting.cpp
  src/simulation/player_input.cpp
  src/simulation/player_armament.cpp
  src/simulation/mission_goals.cpp
  src/simulation/mission_objectives.cpp
  src/simulation/herc_reactions.cpp
  src/simulation/herc_sensors.cpp
  src/simulation/herc_sensors.h
  src/simulation/herc_guard.cpp
  src/simulation/herc_guard.h
  src/simulation/herc_orders.cpp
  src/simulation/herc_orders.h
  src/simulation/herc_strategy.cpp
  src/simulation/herc_strategy.h
  src/simulation/herc_fire_control.cpp
  src/simulation/herc_fire_control.h
  src/simulation/herc_aiming.cpp
  src/simulation/herc_aiming.h
  src/simulation/herc_weapon_selection.cpp
  src/simulation/herc_weapon_selection.h
  src/simulation/herc_assessment.cpp
  src/simulation/herc_assessment.h
  src/simulation/herc_tactics.cpp
  src/simulation/herc_tactics.h
  src/simulation/mission_flyers.cpp
  src/simulation/mission_activity.cpp
  src/simulation/mission_activity.h
  src/simulation/flyer_attack.cpp
  src/simulation/flyer_attack.h
  src/simulation/flyer_navigation.cpp
  src/simulation/flyer_navigation.h
  src/simulation/flyer_motion.cpp
  src/simulation/flyer_motion.h
  src/resources/flyer_definition.cpp
  src/resources/flyer_definition.h
  src/simulation/flyer_actor.cpp
  src/simulation/flyer_actor.h
  src/resources/ground_definition.cpp
  src/resources/ground_definition.h
  src/simulation/ground_actor.cpp
  src/simulation/ground_actor.h
  src/simulation/engagement.cpp
  src/simulation/engagement.h
  src/simulation/controller.cpp
  src/simulation/controller.h
  src/simulation/perception.cpp
  src/simulation/perception.h
  src/simulation/target_selection.cpp
  src/simulation/target_selection.h
  src/simulation/world_projectiles.cpp
  src/simulation/world_projectiles.h
  src/simulation/beam_effect.cpp
  src/simulation/beam_effect.h
  src/render/effect_lighting.cpp
  src/render/effect_lighting.h
  src/simulation/world_effects.cpp
  src/simulation/world_effects.h
  src/simulation/object_pool.h
  src/resources/effects.cpp
  src/resources/effects.h
  src/simulation/effect_state.cpp
  src/simulation/effect_state.h
  src/simulation/hit_dispatch.cpp
  src/simulation/hit_dispatch.h
  src/simulation/projectile_instance.cpp
  src/simulation/projectile_instance.h
  src/simulation/weapon_firing.cpp
  src/simulation/weapon_firing.h
  src/simulation/weapon_bank.cpp
  src/simulation/weapon_bank.h
  src/simulation/herc_actor.cpp
  src/simulation/herc_actor.h
  src/simulation/herc_impact.h
  src/simulation/herc_power.cpp
  src/simulation/herc_power.h
  src/simulation/herc_loadout.cpp
  src/simulation/herc_loadout.h
  src/simulation/hit_volumes.cpp
  src/simulation/hit_volumes.h
  src/resources/projectile_motion.cpp
  src/resources/projectile_motion.h
  src/simulation/projectile_flight.cpp
  src/simulation/projectile_flight.h
  src/simulation/weapon_controls.cpp
  src/simulation/weapon_controls.h
  src/simulation/shield_power.cpp
  src/simulation/shield_power.h
  src/simulation/component_damage.cpp
  src/simulation/component_damage.h
  src/simulation/navigation.cpp
  src/simulation/navigation.h
  src/simulation/mission_triggers.cpp
  src/simulation/mission_triggers.h
  src/simulation/actor_contact.cpp
  src/simulation/actor_contact.h
  src/shell/mission_view.cpp
  src/shell/mission_view.h
  src/simulation/mission_hercs.cpp
  src/simulation/mission_session.cpp
  src/simulation/mission_session.h
  src/resources/name_table.cpp
  src/resources/name_table.h
  src/simulation/camera.cpp
  src/simulation/camera.h
  src/simulation/arcsine_table.h
  src/simulation/keyboard_input.cpp
  src/simulation/frame_clock.cpp
  src/resources/input_options.cpp
  src/simulation/joystick_input.cpp
  src/simulation/joystick_routing.cpp
  src/simulation/keyboard_input.h
  src/simulation/frame_time.cpp
  src/simulation/frame_time.h
  src/resources/herc_definition.cpp
  src/resources/herc_definition.h
  src/simulation/herc_body.cpp
  src/simulation/herc_body.h
  src/resources/model_tree.cpp
  src/resources/model_tree.h
  src/simulation/weapon_assembly.cpp
  src/simulation/weapon_assembly.h
  src/resources/combat.cpp
  src/resources/combat.h
  src/resources/player_roster.cpp
  src/resources/player_roster.h
  src/resources/weapon_mounts.cpp
  src/resources/weapon_mounts.h
  src/simulation/actor_placement.cpp
  src/simulation/actor_placement.h
  src/render/environment_palette.cpp
  src/render/environment_palette.h
  src/render/environment_renderer.cpp
  src/render/environment_renderer.h
  src/render/ground_texture.cpp
  src/render/ground_texture.h
  src/render/clouds.cpp
  src/render/clouds.h
  src/render/ground_marks.cpp
  src/render/ground_marks.h
  src/render/panorama.cpp
  src/render/panorama.h
  src/render/background_bands.cpp
  src/render/background_bands.h
  src/render/scenery_renderer.cpp
  src/render/scenery_renderer.h
  src/render/distance_shading.cpp
  src/render/distance_shading.h
  src/render/scene_order.cpp
  src/render/scene_order.h
  src/resources/world.cpp
  src/resources/world.h
  src/simulation/scenery_world.cpp
  src/simulation/scenery_world.h
  src/simulation/scenery_collision.cpp
  src/simulation/scenery_collision.h
  src/simulation/locomotion.cpp
  src/simulation/locomotion.h
  src/simulation/arctangent_table.h
  src/resources/herc_profile.cpp
  src/resources/herc_profile.h
  src/simulation/aim_control.cpp
  src/simulation/aim_control.h
  src/simulation/animation_rig.cpp
  src/simulation/animation_rig.h
  src/resources/model_bindings.cpp
  src/resources/model_bindings.h
  src/render/bitmap_raster.cpp
  src/render/bitmap_raster.h
  src/render/line_raster.cpp
  src/render/line_raster.h
  src/render/model_renderer.cpp
  src/render/model_renderer.h
  src/render/gouraud_raster.cpp
  src/render/gouraud_raster.h
  src/render/texture_raster.cpp
  src/render/texture_raster.h
  src/render/polygon_raster.cpp
  src/render/polygon_raster.h
  src/render/projection.cpp
  src/render/projection.h
  src/render/palette_shading.cpp
  src/render/palette_shading.h
  src/simulation/fixed_math.cpp
  src/simulation/fixed_math.h
  src/render/model_lighting.cpp
  src/render/model_lighting.h
  src/simulation/animation_channel.cpp
  src/simulation/animation_channel.h
  src/simulation/model_instance.cpp
  src/simulation/model_instance.h
  src/render/model_traversal.cpp
  src/render/model_traversal.h
  src/simulation/model_math.cpp
  src/simulation/model_math.h
  src/simulation/cosine_table.h
  src/resources/model.cpp
  src/resources/model.h
  src/resources/mission_text.cpp
  src/resources/mission_text.h
  src/resources/mission_compiler.cpp
  src/resources/mission_compiler.h
  src/resources/mission_records.h
  src/resources/input_tape.cpp
  src/resources/input_tape.h
  src/resources/mission_script.cpp
  src/resources/mission_script.h
  src/simulation/random_state.cpp
  src/simulation/random_state.h
  src/resources/mission_conditions.cpp
  src/resources/mission_conditions.h
  src/shell/dialog.cpp
  src/shell/dialog.h
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/resources/artwork.cpp
  src/resources/artwork.h
  src/resources/archive.cpp
  src/resources/archive.h
  src/resources/binary_reader.cpp
  src/resources/binary_reader.h
  src/resources/resource_store.cpp
  src/resources/resource_store.h
)
target_link_libraries(earthsiege_core PRIVATE nuked_opl3 MT32Emu::mt32emu sc55_backend tinysoundfont)
target_include_directories(earthsiege_core PUBLIC src "${PROJECT_SOURCE_DIR}")
target_compile_definitions(earthsiege_core PUBLIC VECTORSTORM_NO_BOOST)
target_compile_options(earthsiege_core PUBLIC
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
)

add_executable(earthsiege
  src/main.cpp
  src/platform/music_setup.cpp
  src/platform/joystick_devices.cpp
  src/platform/audio_output.cpp
  src/platform/window.cpp
  src/platform/window.h
  src/platform/paths.cpp
)
target_link_libraries(earthsiege PRIVATE earthsiege_core glfw OpenGL::GL pcm_device)

if(BUILD_TESTING)
  add_executable(mission_alert_test tests/mission_alert_test.cpp)
  target_link_libraries(mission_alert_test PRIVATE earthsiege_core)
  add_test(NAME mission_alert COMMAND mission_alert_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/alert_rasters.bin")
  add_executable(game_shell_test tests/game_shell_test.cpp)
  target_link_libraries(game_shell_test PRIVATE earthsiege_core)
  add_test(NAME game_shell COMMAND game_shell_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_BINARY_DIR}")
  add_executable(campaign_cycles_test tests/campaign_cycles_test.cpp)
  target_link_libraries(campaign_cycles_test PRIVATE earthsiege_core)
  add_test(NAME campaign_cycles COMMAND campaign_cycles_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/campaign_cycles.bin")
  add_test(NAME campaign_journeys COMMAND campaign_cycles_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/campaign_journeys.bin")
  add_executable(game_state_test tests/game_state_test.cpp)
  target_link_libraries(game_state_test PRIVATE earthsiege_core)
  add_test(NAME game_state COMMAND game_state_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/shell_preparation.bin")
  add_executable(spatial_audio_test tests/spatial_audio_test.cpp)
  target_link_libraries(spatial_audio_test PRIVATE earthsiege_core)
  add_test(NAME spatial_audio COMMAND spatial_audio_test "${PROJECT_SOURCE_DIR}/tests/fixtures/spatial_audio.bin")
  add_executable(cockpit_radio_test tests/cockpit_radio_test.cpp)
  target_link_libraries(cockpit_radio_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_radio COMMAND cockpit_radio_test "${PROJECT_SOURCE_DIR}/assets")
  add_executable(wingmate_portrait_test tests/wingmate_portrait_test.cpp)
  target_link_libraries(wingmate_portrait_test PRIVATE earthsiege_core)
  add_test(NAME wingmate_portrait COMMAND wingmate_portrait_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/wingmate_portraits.bin")
  add_executable(pilot_transmission_test tests/pilot_transmission_test.cpp)
  target_link_libraries(pilot_transmission_test PRIVATE earthsiege_core)
  add_test(NAME pilot_transmission COMMAND pilot_transmission_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/portrait_state.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/portrait_failure.bin")
  add_executable(radio_portrait_test tests/radio_portrait_test.cpp)
  target_link_libraries(radio_portrait_test PRIVATE earthsiege_core)
  add_test(NAME radio_portrait COMMAND radio_portrait_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_portraits.bin")
  add_executable(radio_banner_test tests/radio_banner_test.cpp)
  target_link_libraries(radio_banner_test PRIVATE earthsiege_core)
  add_test(NAME radio_banner COMMAND radio_banner_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_banner.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/system_banner.bin")
  add_executable(portrait_sync_test tests/portrait_sync_test.cpp)
  target_link_libraries(portrait_sync_test PRIVATE earthsiege_core)
  add_test(NAME portrait_sync COMMAND portrait_sync_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/portrait_timing.bin")
  add_executable(radio_controller_test tests/radio_controller_test.cpp)
  target_link_libraries(radio_controller_test PRIVATE earthsiege_core)
  add_test(NAME radio_controller COMMAND radio_controller_test "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_state.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_cancel.bin")
  add_executable(radio_catalogue_test tests/radio_catalogue_test.cpp)
  target_link_libraries(radio_catalogue_test PRIVATE earthsiege_core)
  add_test(NAME radio_catalogue COMMAND radio_catalogue_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_descriptors.bin")
  add_executable(mfd_failure_test tests/mfd_failure_test.cpp)
  target_link_libraries(mfd_failure_test PRIVATE earthsiege_core)
  add_test(NAME mfd_failure COMMAND mfd_failure_test "${PROJECT_SOURCE_DIR}/tests/fixtures/mfd_failure.bin")
  add_executable(radio_queue_test tests/radio_queue_test.cpp)
  target_link_libraries(radio_queue_test PRIVATE earthsiege_core)
  add_test(NAME radio_queue COMMAND radio_queue_test "${PROJECT_SOURCE_DIR}/tests/fixtures/radio_queue.bin")
  add_executable(instrument_failure_test tests/instrument_failure_test.cpp)
  target_link_libraries(instrument_failure_test PRIVATE earthsiege_core)
  add_test(NAME instrument_failure COMMAND instrument_failure_test "${PROJECT_SOURCE_DIR}/tests/fixtures/instrument_failure.bin")
  add_executable(capture_cockpit tools/capture_cockpit.cpp)
  target_link_libraries(capture_cockpit PRIVATE earthsiege_core)
  add_executable(mission_orders_test tests/mission_orders_test.cpp)
  target_link_libraries(mission_orders_test PRIVATE earthsiege_core)
  add_test(NAME mission_orders COMMAND mission_orders_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_orders.bin")
  add_executable(wingmate_dispatch_test tests/wingmate_dispatch_test.cpp)
  target_link_libraries(wingmate_dispatch_test PRIVATE earthsiege_core)
  add_test(NAME wingmate_dispatch COMMAND wingmate_dispatch_test "${PROJECT_SOURCE_DIR}/tests/fixtures/wingmate_dispatch.bin")
  add_executable(wingmate_orders_test tests/wingmate_orders_test.cpp)
  target_link_libraries(wingmate_orders_test PRIVATE earthsiege_core)
  add_test(NAME wingmate_orders COMMAND wingmate_orders_test "${PROJECT_SOURCE_DIR}/tests/fixtures/wingmate_orders.bin")
  add_executable(mission_displays_test tests/mission_displays_test.cpp)
  target_link_libraries(mission_displays_test PRIVATE earthsiege_core)
  add_test(NAME mission_displays COMMAND mission_displays_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_displays.bin")
  add_executable(mfd_controller_test tests/mfd_controller_test.cpp)
  target_link_libraries(mfd_controller_test PRIVATE earthsiege_core)
  add_test(NAME mfd_controller COMMAND mfd_controller_test "${PROJECT_SOURCE_DIR}/tests/fixtures/mfd_state.bin")
  add_executable(missile_display_test tests/missile_display_test.cpp)
  target_link_libraries(missile_display_test PRIVATE earthsiege_core)
  add_test(NAME missile_ground COMMAND missile_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/missile_ground.bin")
  add_test(NAME missile_displays COMMAND missile_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/missile_displays.bin")
  add_executable(navigation_display_test tests/navigation_display_test.cpp)
  target_link_libraries(navigation_display_test PRIVATE earthsiege_core)
  add_test(NAME navigation_displays COMMAND navigation_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/navigation_displays.bin")
  add_executable(communications_display_test tests/communications_display_test.cpp)
  target_link_libraries(communications_display_test PRIVATE earthsiege_core)
  add_test(NAME communications_displays COMMAND communications_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/communications_displays.bin")
  add_executable(scanner_display_test tests/scanner_display_test.cpp)
  target_link_libraries(scanner_display_test PRIVATE earthsiege_core)
  add_test(NAME scanner_displays COMMAND scanner_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/scanner_displays.bin")
  add_executable(mfd_instruments_test tests/mfd_instruments_test.cpp)
  target_link_libraries(mfd_instruments_test PRIVATE earthsiege_core)
  add_test(NAME mfd_instruments COMMAND mfd_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mfd_controls.bin")
  add_executable(damage_display_test tests/damage_display_test.cpp)
  target_link_libraries(damage_display_test PRIVATE earthsiege_core)
  add_test(NAME damage_displays COMMAND damage_display_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/damage_displays.bin")
  add_executable(presentation_test tests/presentation_test.cpp)
  target_link_libraries(presentation_test PRIVATE earthsiege_core)
  add_test(NAME presentations COMMAND presentation_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/presentation_programs.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/presentation_frames.bin")
  add_executable(clock_instrument_test tests/clock_instrument_test.cpp)
  target_link_libraries(clock_instrument_test PRIVATE earthsiege_core)
  add_test(NAME clock_instruments COMMAND clock_instrument_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/clock_instruments.bin")
  add_executable(mission_instruments_test tests/mission_instruments_test.cpp)
  target_link_libraries(mission_instruments_test PRIVATE earthsiege_core)
  add_test(NAME mission_instruments COMMAND mission_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_instruments.bin")
  add_executable(waypoint_instruments_test tests/waypoint_instruments_test.cpp)
  target_link_libraries(waypoint_instruments_test PRIVATE earthsiege_core)
  add_test(NAME waypoint_instruments COMMAND waypoint_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/waypoint_instruments.bin")
  add_executable(target_instruments_test tests/target_instruments_test.cpp)
  target_link_libraries(target_instruments_test PRIVATE earthsiege_core)
  add_test(NAME target_instruments COMMAND target_instruments_test "${PROJECT_SOURCE_DIR}/tests/fixtures/target_instruments.bin")
  add_executable(navigation_instruments_test tests/navigation_instruments_test.cpp)
  target_link_libraries(navigation_instruments_test PRIVATE earthsiege_core)
  add_test(NAME navigation_instruments COMMAND navigation_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/navigation_instruments.bin")
  add_executable(pointer_controls_test tests/pointer_controls_test.cpp)
  target_link_libraries(pointer_controls_test PRIVATE earthsiege_core)
  add_test(NAME pointer_controls COMMAND pointer_controls_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/pointer_controls.bin")
  add_executable(weapon_group_test tests/weapon_group_test.cpp)
  target_link_libraries(weapon_group_test PRIVATE earthsiege_core)
  add_test(NAME weapon_groups COMMAND weapon_group_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_groups.bin")
  add_executable(weapon_display_test tests/weapon_display_test.cpp)
  target_link_libraries(weapon_display_test PRIVATE earthsiege_core)
  add_test(NAME weapon_display COMMAND weapon_display_test "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_display.bin")
  add_executable(weapon_instruments_test tests/weapon_instruments_test.cpp)
  target_link_libraries(weapon_instruments_test PRIVATE earthsiege_core)
  add_test(NAME weapon_instruments COMMAND weapon_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_instruments.bin")
  add_executable(cockpit_page_copy_test tests/cockpit_page_copy_test.cpp)
  target_link_libraries(cockpit_page_copy_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_page_copy COMMAND cockpit_page_copy_test "${PROJECT_SOURCE_DIR}/tests/fixtures/cockpit_page_copy.bin")
  add_executable(weapon_charge_retained_test tests/weapon_charge_retained_test.cpp)
  target_link_libraries(weapon_charge_retained_test PRIVATE earthsiege_core)
  add_test(NAME weapon_charge_retained COMMAND weapon_charge_retained_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_charge_retained.bin")
  add_executable(shield_instruments_test tests/shield_instruments_test.cpp)
  target_link_libraries(shield_instruments_test PRIVATE earthsiege_core)
  add_test(NAME shield_instruments COMMAND shield_instruments_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/shield_instruments.bin")
  add_executable(throttle_gauge_test tests/throttle_gauge_test.cpp)
  target_link_libraries(throttle_gauge_test PRIVATE earthsiege_core)
  add_test(NAME throttle_gauges COMMAND throttle_gauge_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/throttle_gauges.bin")
  add_executable(throttle_updates_test tests/throttle_updates_test.cpp)
  target_link_libraries(throttle_updates_test PRIVATE earthsiege_core)
  add_test(NAME throttle_updates COMMAND throttle_updates_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/throttle_updates.bin")
  add_executable(reactor_gauge_test tests/reactor_gauge_test.cpp)
  target_link_libraries(reactor_gauge_test PRIVATE earthsiege_core)
  add_test(NAME reactor_gauges COMMAND reactor_gauge_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/reactor_gauges.bin")
  add_executable(cockpit_scene_test tests/cockpit_scene_test.cpp)
  target_link_libraries(cockpit_scene_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_scenery COMMAND cockpit_scene_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/cockpit_scenery.bin")
  add_executable(cockpit_artwork_test tests/cockpit_artwork_test.cpp)
  target_link_libraries(cockpit_artwork_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_artwork COMMAND cockpit_artwork_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/cockpit_artwork.bin")
  add_executable(cockpit_mask_test tests/cockpit_mask_test.cpp)
  target_link_libraries(cockpit_mask_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_masks COMMAND cockpit_mask_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/cockpit_masks.bin")
  add_executable(player_targeting_test tests/player_targeting_test.cpp)
  target_link_libraries(player_targeting_test PRIVATE earthsiege_core)
  add_test(NAME player_targeting COMMAND player_targeting_test "${PROJECT_SOURCE_DIR}/tests/fixtures/player_targeting.bin")
  add_executable(player_input_test tests/player_input_test.cpp)
  target_link_libraries(player_input_test PRIVATE earthsiege_core)
  add_test(NAME player_input COMMAND player_input_test "${PROJECT_SOURCE_DIR}/tests/fixtures/player_input.bin")
  add_executable(player_armament_test tests/player_armament_test.cpp)
  target_link_libraries(player_armament_test PRIVATE earthsiege_core)
  add_test(NAME player_armament COMMAND player_armament_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/player_armament.bin")
  add_executable(mission_objectives_test tests/mission_objectives_test.cpp)
  target_link_libraries(mission_objectives_test PRIVATE earthsiege_core)
  add_test(NAME mission_objectives COMMAND mission_objectives_test "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_objectives.bin")
  add_executable(herc_reactions_test tests/herc_reactions_test.cpp)
  target_link_libraries(herc_reactions_test PRIVATE earthsiege_core)
  add_test(NAME herc_reactions COMMAND herc_reactions_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_reactions.bin")
  add_executable(herc_sensors_test tests/herc_sensors_test.cpp)
  target_link_libraries(herc_sensors_test PRIVATE earthsiege_core)
  add_test(NAME herc_sensors COMMAND herc_sensors_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_sensors.bin")
  add_executable(herc_guard_test tests/herc_guard_test.cpp)
  target_link_libraries(herc_guard_test PRIVATE earthsiege_core)
  add_test(NAME herc_guard COMMAND herc_guard_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_guard.bin")
  add_executable(herc_auxiliary_test tests/herc_auxiliary_test.cpp)
  target_link_libraries(herc_auxiliary_test PRIVATE earthsiege_core)
  add_test(NAME herc_auxiliary COMMAND herc_auxiliary_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_auxiliary.bin")
  add_executable(herc_strategy_test tests/herc_strategy_test.cpp)
  target_link_libraries(herc_strategy_test PRIVATE earthsiege_core)
  add_test(NAME herc_strategy COMMAND herc_strategy_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_strategy.bin")
  add_executable(herc_fire_control_test tests/herc_fire_control_test.cpp)
  target_link_libraries(herc_fire_control_test PRIVATE earthsiege_core)
  add_test(NAME herc_fire_control COMMAND herc_fire_control_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_fire_control.bin")
  add_executable(herc_target_aim_test tests/herc_target_aim_test.cpp)
  target_link_libraries(herc_target_aim_test PRIVATE earthsiege_core)
  add_test(NAME herc_target_aim COMMAND herc_target_aim_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_target_aim.bin")
  add_executable(herc_aimed_body_test tests/herc_aimed_body_test.cpp)
  target_link_libraries(herc_aimed_body_test PRIVATE earthsiege_core)
  add_test(NAME herc_aimed_body COMMAND herc_aimed_body_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_aimed_body.bin")
  add_executable(herc_aiming_test tests/herc_aiming_test.cpp)
  target_link_libraries(herc_aiming_test PRIVATE earthsiege_core)
  add_test(NAME herc_aiming COMMAND herc_aiming_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_aiming.bin")
  add_executable(herc_weapon_choice_test tests/herc_weapon_choice_test.cpp)
  target_link_libraries(herc_weapon_choice_test PRIVATE earthsiege_core)
  add_test(NAME herc_weapon_choice COMMAND herc_weapon_choice_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_weapon_choice.bin")
  add_executable(herc_assessment_test tests/herc_assessment_test.cpp)
  target_link_libraries(herc_assessment_test PRIVATE earthsiege_core)
  add_test(NAME herc_assessment COMMAND herc_assessment_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_assessment.bin")
  add_executable(herc_tactics_test tests/herc_tactics_test.cpp)
  target_link_libraries(herc_tactics_test PRIVATE earthsiege_core)
  add_test(NAME herc_tactics COMMAND herc_tactics_test "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_tactics.bin")
  add_executable(mission_world_frames_test tests/mission_world_frames_test.cpp)
  target_link_libraries(mission_world_frames_test PRIVATE earthsiege_core)
  add_test(NAME mission_world_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_world_frames.bin")
  add_test(NAME mission_authored_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_authored_frames.bin")
  add_test(NAME mission_career_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_career_frames.bin")
  add_test(NAME mission_authored_player_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_authored_player_frames.bin")
  add_test(NAME mission_player_disabled_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_player_disabled_frames.bin")
  add_test(NAME mission_corrected_offset_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_corrected_offset_frames.bin")
  add_test(NAME mission_initialized_aim_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_initialized_aim_frames.bin")
  add_test(NAME mission_disabled_reverse_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_disabled_reverse_frames.bin")
  add_test(NAME mission_ramming_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_ramming_frames.bin")
  add_executable(target_geometry_test tests/target_geometry_test.cpp)
  target_link_libraries(target_geometry_test PRIVATE earthsiege_core)
  add_test(NAME target_geometry COMMAND target_geometry_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/target_geometry.bin")
  add_executable(destruction_counters_test tests/destruction_counters_test.cpp)
  target_link_libraries(destruction_counters_test PRIVATE earthsiege_core)
  add_test(NAME destruction_counters COMMAND destruction_counters_test "${PROJECT_SOURCE_DIR}/tests/fixtures/destruction_counters.bin")
  add_test(NAME mission_target_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_target_frames.bin")
  add_test(NAME mission_manual_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_manual_frames.bin")
  add_test(NAME mission_player_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_player_frames.bin")
  add_test(NAME mission_seeded_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_seeded_frames.bin")
  add_test(NAME mission_combat_frames COMMAND mission_world_frames_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_combat_frames.bin")
  add_executable(mission_activity_test tests/mission_activity_test.cpp)
  target_link_libraries(mission_activity_test PRIVATE earthsiege_core)
  add_test(NAME mission_activity COMMAND mission_activity_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_activity.bin")
  add_executable(engagement_test tests/engagement_test.cpp)
  target_link_libraries(engagement_test PRIVATE earthsiege_core)
  add_test(NAME engagement COMMAND engagement_test "${PROJECT_SOURCE_DIR}/tests/fixtures/engagement.bin")
  add_executable(controller_test tests/controller_test.cpp)
  target_link_libraries(controller_test PRIVATE earthsiege_core)
  add_test(NAME controllers COMMAND controller_test "${PROJECT_SOURCE_DIR}/tests/fixtures/controllers.bin")
  add_executable(perception_test tests/perception_test.cpp)
  target_link_libraries(perception_test PRIVATE earthsiege_core)
  add_test(NAME perception COMMAND perception_test "${PROJECT_SOURCE_DIR}/tests/fixtures/perception.bin")
  add_executable(target_selection_test tests/target_selection_test.cpp)
  target_link_libraries(target_selection_test PRIVATE earthsiege_core)
  add_test(NAME target_selection COMMAND target_selection_test "${PROJECT_SOURCE_DIR}/tests/fixtures/target_selection.bin")
  add_executable(mission_combat_test tests/mission_combat_test.cpp)
  target_link_libraries(mission_combat_test PRIVATE earthsiege_core)
  add_test(NAME mission_combat COMMAND mission_combat_test "${PROJECT_SOURCE_DIR}/assets")
  add_executable(projectile_world_test tests/projectile_world_test.cpp)
  target_link_libraries(projectile_world_test PRIVATE earthsiege_core)
  add_test(NAME projectile_world COMMAND projectile_world_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/projectile_world.bin")
  add_executable(beam_effect_test tests/beam_effect_test.cpp)
  target_link_libraries(beam_effect_test PRIVATE earthsiege_core)
  add_test(NAME beam_effect COMMAND beam_effect_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/beam_effects.bin")
  add_executable(effect_render_test tests/effect_render_test.cpp)
  target_link_libraries(effect_render_test PRIVATE earthsiege_core)
  add_test(NAME effect_render COMMAND effect_render_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/effect_render.bin")
  add_executable(effect_lighting_test tests/effect_lighting_test.cpp)
  target_link_libraries(effect_lighting_test PRIVATE earthsiege_core)
  add_test(NAME effect_lighting COMMAND effect_lighting_test "${PROJECT_SOURCE_DIR}/tests/fixtures/effect_lighting.bin")
  add_executable(camera_test tests/camera_test.cpp)
  target_link_libraries(camera_test PRIVATE earthsiege_core)
  add_test(NAME camera COMMAND camera_test "${PROJECT_SOURCE_DIR}/tests/fixtures/camera.bin")
  add_executable(input_test tests/input_test.cpp)
  target_link_libraries(input_test PRIVATE earthsiege_core)
  add_test(NAME input COMMAND input_test "${PROJECT_SOURCE_DIR}/tests/fixtures/input.bin")
  add_executable(component_hits_test tests/component_hits_test.cpp)
  target_link_libraries(component_hits_test PRIVATE earthsiege_core)
  add_test(NAME component_hits COMMAND component_hits_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/component_hits.bin")
  add_executable(hit_volumes_test tests/hit_volumes_test.cpp)
  target_link_libraries(hit_volumes_test PRIVATE earthsiege_core)
  add_test(NAME hit_volumes COMMAND hit_volumes_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/hit_volumes.bin")
  add_executable(effect_world_test tests/effect_world_test.cpp)
  target_link_libraries(effect_world_test PRIVATE earthsiege_core)
  add_test(NAME effect_world COMMAND effect_world_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/effect_world.bin")
  add_executable(effect_lifetime_test tests/effect_lifetime_test.cpp)
  target_link_libraries(effect_lifetime_test PRIVATE earthsiege_core)
  add_test(NAME effect_lifetimes COMMAND effect_lifetime_test "${PROJECT_SOURCE_DIR}/tests/fixtures/debris_motion.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/object_pools.bin")
  add_executable(persistent_effects_test tests/persistent_effects_test.cpp)
  target_link_libraries(persistent_effects_test PRIVATE earthsiege_core)
  add_test(NAME persistent_effects COMMAND persistent_effects_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/smoke.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/detached_weapons.bin")
  add_executable(effects_test tests/effects_test.cpp)
  target_link_libraries(effects_test PRIVATE earthsiege_core)
  add_test(NAME effects COMMAND effects_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/effects.bin")
  add_executable(herc_damage_test tests/herc_damage_test.cpp)
  target_link_libraries(herc_damage_test PRIVATE earthsiege_core)
  add_test(NAME herc_damage COMMAND herc_damage_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_damage.bin")
  add_executable(perception_world_test tests/perception_world_test.cpp)
  target_link_libraries(perception_world_test PRIVATE earthsiege_core)
  add_test(NAME perception_world COMMAND perception_world_test "${PROJECT_SOURCE_DIR}/tests/fixtures/perception_world.bin")
  add_executable(flyer_attack_test tests/flyer_attack_test.cpp)
  target_link_libraries(flyer_attack_test PRIVATE earthsiege_core)
  add_test(NAME flyer_attack COMMAND flyer_attack_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/flyer_attack.bin")
  add_executable(flyer_navigation_test tests/flyer_navigation_test.cpp)
  target_link_libraries(flyer_navigation_test PRIVATE earthsiege_core)
  add_test(NAME flyer_navigation COMMAND flyer_navigation_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/flyer_navigation.bin")
  add_executable(flyer_motion_test tests/flyer_motion_test.cpp)
  target_link_libraries(flyer_motion_test PRIVATE earthsiege_core)
  add_test(NAME flyer_motion COMMAND flyer_motion_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/flyer_motion.bin")
  add_executable(mission_flyers_test tests/mission_flyers_test.cpp)
  target_link_libraries(mission_flyers_test PRIVATE earthsiege_core)
  add_test(NAME mission_flyers COMMAND mission_flyers_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_flyers.bin")
  add_executable(flyer_actors_test tests/flyer_actors_test.cpp)
  target_link_libraries(flyer_actors_test PRIVATE earthsiege_core)
  add_test(NAME flyer_actors COMMAND flyer_actors_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/flyer_actors.bin")
  add_executable(turret_firing_test tests/turret_firing_test.cpp)
  target_link_libraries(turret_firing_test PRIVATE earthsiege_core)
  add_test(NAME turret_firing COMMAND turret_firing_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/turret_firing.bin")
  add_executable(turret_tracking_test tests/turret_tracking_test.cpp)
  target_link_libraries(turret_tracking_test PRIVATE earthsiege_core)
  add_test(NAME turret_tracking COMMAND turret_tracking_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/turret_tracking.bin")
  add_executable(ground_combat_test tests/ground_combat_test.cpp)
  target_link_libraries(ground_combat_test PRIVATE earthsiege_core)
  add_test(NAME ground_combat COMMAND ground_combat_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/ground_combat.bin")
  add_executable(ground_actors_test tests/ground_actors_test.cpp)
  target_link_libraries(ground_actors_test PRIVATE earthsiege_core)
  add_test(NAME ground_actors COMMAND ground_actors_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/ground_actors.bin")
  add_executable(mission_hercs_test tests/mission_hercs_test.cpp)
  target_link_libraries(mission_hercs_test PRIVATE earthsiege_core)
  add_test(NAME mission_hercs COMMAND mission_hercs_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mission_hercs.bin")
  add_executable(starting_damage_test tests/starting_damage_test.cpp)
  target_link_libraries(starting_damage_test PRIVATE earthsiege_core)
  add_test(NAME starting_damage COMMAND starting_damage_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/starting_damage.bin")
  add_executable(herc_impacts_test tests/herc_impacts_test.cpp)
  target_link_libraries(herc_impacts_test PRIVATE earthsiege_core)
  add_test(NAME herc_impacts COMMAND herc_impacts_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_impacts.bin")
  add_executable(world_impacts_test tests/world_impacts_test.cpp)
  target_link_libraries(world_impacts_test PRIVATE earthsiege_core)
  add_test(NAME world_impacts COMMAND world_impacts_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/world_impacts.bin")
  add_executable(hit_dispatch_test tests/hit_dispatch_test.cpp)
  target_link_libraries(hit_dispatch_test PRIVATE earthsiege_core)
  add_test(NAME hit_dispatch COMMAND hit_dispatch_test "${PROJECT_SOURCE_DIR}/tests/fixtures/hit_dispatch.bin")
  add_executable(projectile_instance_test tests/projectile_instance_test.cpp)
  target_link_libraries(projectile_instance_test PRIVATE earthsiege_core)
  add_test(NAME projectile_instances COMMAND projectile_instance_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/projectile_packets.bin")
  add_executable(projectile_flight_test tests/projectile_flight_test.cpp)
  target_link_libraries(projectile_flight_test PRIVATE earthsiege_core)
  add_test(NAME projectile_flight COMMAND projectile_flight_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/projectile_flight.bin")
  add_executable(weapon_firing_test tests/weapon_firing_test.cpp)
  target_link_libraries(weapon_firing_test PRIVATE earthsiege_core)
  add_test(NAME weapon_firing COMMAND weapon_firing_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_firing.bin")
  add_executable(weapon_bank_test tests/weapon_bank_test.cpp)
  target_link_libraries(weapon_bank_test PRIVATE earthsiege_core)
  add_test(NAME weapon_banks COMMAND weapon_bank_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_banks.bin")
  add_executable(weapon_controls_test tests/weapon_controls_test.cpp)
  target_link_libraries(weapon_controls_test PRIVATE earthsiege_core)
  add_test(NAME weapon_controls COMMAND weapon_controls_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_controls.bin")
  add_executable(damaged_renderer_test tests/damaged_renderer_test.cpp)
  target_link_libraries(damaged_renderer_test PRIVATE earthsiege_core)
  add_test(NAME damaged_renderer COMMAND damaged_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/damaged_render.bin")
  add_executable(damage_test tests/damage_test.cpp)
  target_link_libraries(damage_test PRIVATE earthsiege_core)
  add_test(NAME damage COMMAND damage_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/damage.bin")
  add_executable(navigation_test tests/navigation_test.cpp)
  target_link_libraries(navigation_test PRIVATE earthsiege_core)
  add_test(NAME navigation COMMAND navigation_test "${PROJECT_SOURCE_DIR}/tests/fixtures/navigation.bin")
  add_executable(trigger_test tests/trigger_test.cpp)
  target_link_libraries(trigger_test PRIVATE earthsiege_core)
  add_test(NAME triggers COMMAND trigger_test "${PROJECT_SOURCE_DIR}/tests/fixtures/triggers.bin")
  add_executable(contact_test tests/contact_test.cpp)
  target_link_libraries(contact_test PRIVATE earthsiege_core)
  add_test(NAME contacts COMMAND contact_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/actor_contact.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/scenery_contact.bin")
  add_executable(mixed_scene_test tests/mixed_scene_test.cpp)
  target_link_libraries(mixed_scene_test PRIVATE earthsiege_core)
  add_test(NAME mixed_scene COMMAND mixed_scene_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/mixed_scene.bin")
  add_executable(herc_body_test tests/herc_body_test.cpp)
  target_link_libraries(herc_body_test PRIVATE earthsiege_core)
  add_test(NAME herc_damaged_motion COMMAND herc_body_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_damaged.bin")
  add_test(NAME herc_requested_motion COMMAND herc_body_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_request.bin")
  add_test(NAME contact_motion COMMAND herc_body_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/contact_motion.bin")
  add_test(NAME herc_body COMMAND herc_body_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/herc_body.bin")
  add_executable(actor_detail_test tests/actor_detail_test.cpp)
  target_link_libraries(actor_detail_test PRIVATE earthsiege_core)
  add_test(NAME actor_detail COMMAND actor_detail_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/actor_detail.bin")
  add_executable(armed_renderer_test tests/armed_renderer_test.cpp)
  target_link_libraries(armed_renderer_test PRIVATE earthsiege_core)
  add_test(NAME armed_renderer COMMAND armed_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/armed_render.bin")
  add_executable(weapon_assembly_test tests/weapon_assembly_test.cpp)
  target_link_libraries(weapon_assembly_test PRIVATE earthsiege_core)
  add_test(NAME weapon_assembly COMMAND weapon_assembly_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_assembly.bin")
  add_executable(combat_test tests/combat_test.cpp)
  target_link_libraries(combat_test PRIVATE earthsiege_core)
  add_test(NAME combat_resources COMMAND combat_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/combat.bin")
  add_executable(actor_placement_test tests/actor_placement_test.cpp)
  target_link_libraries(actor_placement_test PRIVATE earthsiege_core)
  add_test(NAME actor_placement COMMAND actor_placement_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/actor_placement.bin")
  add_executable(background_bands_test tests/background_bands_test.cpp)
  target_link_libraries(background_bands_test PRIVATE earthsiege_core)
  add_test(NAME background_bands COMMAND background_bands_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/background_bands.bin")
  add_executable(scenery_renderer_test tests/scenery_renderer_test.cpp)
  target_link_libraries(scenery_renderer_test PRIVATE earthsiege_core)
  add_test(NAME scenery_renderer COMMAND scenery_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/scenery_render.bin")
  add_executable(distance_shading_test tests/distance_shading_test.cpp)
  target_link_libraries(distance_shading_test PRIVATE earthsiege_core)
  add_test(NAME distance_shading COMMAND distance_shading_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/distance_shading.bin")
  add_executable(scene_order_test tests/scene_order_test.cpp)
  target_link_libraries(scene_order_test PRIVATE earthsiege_core)
  add_test(NAME scene_order COMMAND scene_order_test "${PROJECT_SOURCE_DIR}/tests/fixtures/scene_order.bin")
  add_test(NAME expansion_placements COMMAND actor_placement_test "${PROJECT_SOURCE_DIR}/assets/expansion" "${PROJECT_SOURCE_DIR}/tests/fixtures/expansion_placements.bin" expansion)
  add_executable(expansion_resources_test tests/expansion_resources_test.cpp)
  target_link_libraries(expansion_resources_test PRIVATE earthsiege_core)
  add_test(NAME expansion_resources COMMAND expansion_resources_test "${PROJECT_SOURCE_DIR}/assets/expansion" "${PROJECT_SOURCE_DIR}/tests/fixtures/expansion.bin")
  add_executable(world_test tests/world_test.cpp)
  target_link_libraries(world_test PRIVATE earthsiege_core)
  add_test(NAME worlds COMMAND world_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/world.bin")
  add_executable(scenery_collision_test tests/scenery_collision_test.cpp)
  target_link_libraries(scenery_collision_test PRIVATE earthsiege_core)
  add_test(NAME scenery_collision COMMAND scenery_collision_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/scenery_collision.bin")
  add_executable(locomotion_test tests/locomotion_test.cpp)
  target_link_libraries(locomotion_test PRIVATE earthsiege_core)
  add_test(NAME locomotion COMMAND locomotion_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/locomotion.bin" "${PROJECT_SOURCE_DIR}/tests/fixtures/direction_angles.bin")
  add_executable(aim_control_test tests/aim_control_test.cpp)
  target_link_libraries(aim_control_test PRIVATE earthsiege_core)
  add_test(NAME aim_controls COMMAND aim_control_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/aim_control.bin")
  add_executable(animation_rig_test tests/animation_rig_test.cpp)
  target_link_libraries(animation_rig_test PRIVATE earthsiege_core)
  add_test(NAME animation_rigs COMMAND animation_rig_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/animation_rigs.bin")
  add_executable(model_bindings_test tests/model_bindings_test.cpp)
  target_link_libraries(model_bindings_test PRIVATE earthsiege_core)
  add_test(NAME model_bindings COMMAND model_bindings_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/model_bindings.bin")
  add_executable(bitmap_raster_test tests/bitmap_raster_test.cpp)
  target_link_libraries(bitmap_raster_test PRIVATE earthsiege_core)
  add_test(NAME bitmap_raster COMMAND bitmap_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/bitmap_raster.bin")
  add_executable(masked_lines_test tests/masked_lines_test.cpp)
  target_link_libraries(masked_lines_test PRIVATE earthsiege_core)
  add_test(NAME masked_lines COMMAND masked_lines_test "${PROJECT_SOURCE_DIR}/tests/fixtures/masked_lines.bin")
  add_executable(line_raster_test tests/line_raster_test.cpp)
  target_link_libraries(line_raster_test PRIVATE earthsiege_core)
  add_test(NAME line_raster COMMAND line_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/line_raster.bin")
  add_executable(model_bounds_test tests/model_bounds_test.cpp)
  target_link_libraries(model_bounds_test PRIVATE earthsiege_core)
  add_test(NAME model_bounds COMMAND model_bounds_test "${PROJECT_SOURCE_DIR}/tests/fixtures/model_bounds.bin")
  add_executable(model_renderer_test tests/model_renderer_test.cpp)
  target_link_libraries(model_renderer_test PRIVATE earthsiege_core)
  add_test(NAME model_renderer COMMAND model_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/model_render.bin")
  add_test(NAME model_light_cache COMMAND model_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/model_light_cache.bin")
  add_executable(gouraud_raster_test tests/gouraud_raster_test.cpp)
  target_link_libraries(gouraud_raster_test PRIVATE earthsiege_core)
  add_test(NAME gouraud_raster COMMAND gouraud_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/gouraud_raster.bin")
  add_executable(texture_raster_test tests/texture_raster_test.cpp)
  target_link_libraries(texture_raster_test PRIVATE earthsiege_core)
  add_test(NAME texture_raster COMMAND texture_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/texture_raster.bin")
  add_executable(polygon_raster_test tests/polygon_raster_test.cpp)
  target_link_libraries(polygon_raster_test PRIVATE earthsiege_core)
  add_test(NAME polygon_raster COMMAND polygon_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/polygon_raster.bin")
  add_executable(projection_test tests/projection_test.cpp)
  target_link_libraries(projection_test PRIVATE earthsiege_core)
  add_test(NAME projection COMMAND projection_test "${PROJECT_SOURCE_DIR}/tests/fixtures/projection.bin")
  add_executable(palette_shading_test tests/palette_shading_test.cpp)
  target_link_libraries(palette_shading_test PRIVATE earthsiege_core)
  add_test(NAME palette_shading COMMAND palette_shading_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/palette_shading.bin")
  add_executable(model_lighting_test tests/model_lighting_test.cpp)
  target_link_libraries(model_lighting_test PRIVATE earthsiege_core)
  add_test(NAME model_lighting COMMAND model_lighting_test "${PROJECT_SOURCE_DIR}/tests/fixtures/model_lighting.bin")
  add_executable(animation_channel_test tests/animation_channel_test.cpp)
  target_link_libraries(animation_channel_test PRIVATE earthsiege_core)
  add_test(NAME animation_channels COMMAND animation_channel_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/animation_channels.bin")
  add_executable(model_instance_test tests/model_instance_test.cpp)
  target_link_libraries(model_instance_test PRIVATE earthsiege_core)
  add_test(NAME model_instances COMMAND model_instance_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/model_poses.bin")
  add_executable(model_traversal_test tests/model_traversal_test.cpp)
  target_link_libraries(model_traversal_test PRIVATE earthsiege_core)
  add_test(NAME model_traversal COMMAND model_traversal_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/model_traversal_checks.tsv")
  add_executable(model_math_test tests/model_math_test.cpp)
  target_link_libraries(model_math_test PRIVATE earthsiege_core)
  add_test(NAME model_math COMMAND model_math_test "${PROJECT_SOURCE_DIR}/tests/fixtures/model_math.bin")
  add_executable(model_test tests/model_test.cpp)
  target_link_libraries(model_test PRIVATE earthsiege_core)
  add_test(NAME models COMMAND model_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/model_checks.tsv")
  add_executable(mission_text_test tests/mission_text_test.cpp)
  target_link_libraries(mission_text_test PRIVATE earthsiege_core)
  add_test(NAME mission_text COMMAND mission_text_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/mission_text_checks.tsv")
  add_executable(script_test tests/script_test.cpp)
  target_link_libraries(script_test PRIVATE earthsiege_core)
  add_test(NAME mission_handoffs COMMAND script_test "${PROJECT_SOURCE_DIR}/tests" "${PROJECT_SOURCE_DIR}/assets")
  add_executable(condition_test tests/condition_test.cpp)
  target_link_libraries(condition_test PRIVATE earthsiege_core)
  add_test(NAME conditions COMMAND condition_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/condition_checks.tsv" "${PROJECT_SOURCE_DIR}/tests/random_checks.tsv")
  add_executable(artwork_test tests/artwork_test.cpp)
  target_link_libraries(artwork_test PRIVATE earthsiege_core)
  add_test(NAME artwork COMMAND artwork_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/artwork_checks.tsv")
  add_executable(resource_test tests/resource_test.cpp)
  target_link_libraries(resource_test PRIVATE earthsiege_core)
  add_test(NAME resources COMMAND resource_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/resource_checks.tsv")
endif()

if(BUILD_TESTING)
  add_executable(pcm_sound_test tests/pcm_sound_test.cpp)
  target_link_libraries(pcm_sound_test PRIVATE earthsiege_core)
  add_test(NAME pcm_sound COMMAND pcm_sound_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pcm_sound.bin)
endif()

if(BUILD_TESTING)
  add_executable(pcm_mixer_test tests/pcm_mixer_test.cpp)
  target_link_libraries(pcm_mixer_test PRIVATE earthsiege_core)
  add_test(NAME pcm_mixer COMMAND pcm_mixer_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pcm_mixer.bin ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pcm_dma.bin)
endif()

if(BUILD_TESTING)
  add_executable(mission_audio_test tests/mission_audio_test.cpp)
  target_link_libraries(mission_audio_test PRIVATE earthsiege_core)
  add_test(NAME mission_audio COMMAND mission_audio_test ${CMAKE_CURRENT_SOURCE_DIR}/assets)
endif()

if(BUILD_TESTING)
  add_executable(footsteps_test tests/footsteps_test.cpp)
  target_link_libraries(footsteps_test PRIVATE earthsiege_core)
  add_test(NAME footsteps COMMAND footsteps_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/footsteps.bin)
endif()

if(BUILD_TESTING)
  add_executable(herc_steps_test tests/herc_steps_test.cpp)
  target_link_libraries(herc_steps_test PRIVATE earthsiege_core)
  add_test(NAME herc_steps COMMAND herc_steps_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/herc_steps.bin)
endif()

if(BUILD_TESTING)
  add_executable(audio_controls_test tests/audio_controls_test.cpp)
  target_link_libraries(audio_controls_test PRIVATE earthsiege_core)
  add_test(NAME audio_controls COMMAND audio_controls_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/audio_controls.bin)
endif()

if(BUILD_TESTING)
  add_executable(hmp_test tests/hmp_test.cpp)
  target_link_libraries(hmp_test PRIVATE earthsiege_core)
  add_test(NAME hmp COMMAND hmp_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hmp.bin)
endif()

if(BUILD_TESTING)
  add_executable(hmi_fm_test tests/hmi_fm_test.cpp)
  target_link_libraries(hmi_fm_test PRIVATE earthsiege_core)
  add_test(NAME hmi_fm COMMAND hmi_fm_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fm_driver.bin)
endif()

if(BUILD_TESTING)
  add_executable(hmi_pcm_test tests/hmi_pcm_test.cpp)
  target_link_libraries(hmi_pcm_test PRIVATE earthsiege_core)
  add_test(NAME hmi_pcm COMMAND hmi_pcm_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pcm_midi.bin)
endif()

if(BUILD_TESTING)
  add_executable(midi_gain_test tests/midi_gain_test.cpp)
  target_link_libraries(midi_gain_test PRIVATE earthsiege_core)
  add_test(NAME midi_gain COMMAND midi_gain_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/music_gain.bin)
endif()

if(BUILD_TESTING)
  add_executable(music_pipeline_test tests/music_pipeline_test.cpp)
  target_link_libraries(music_pipeline_test PRIVATE earthsiege_core)
  add_test(NAME music_pipeline COMMAND music_pipeline_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/music_pipeline.bin)
endif()

if(BUILD_TESTING)
  add_executable(combat_music_test tests/combat_music_test.cpp)
  target_link_libraries(combat_music_test PRIVATE earthsiege_core)
  add_test(NAME combat_music COMMAND combat_music_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_music.bin)
endif()

if(BUILD_TESTING)
  add_executable(music_loops_test tests/music_loops_test.cpp)
  target_link_libraries(music_loops_test PRIVATE earthsiege_core)
  add_test(NAME music_loops COMMAND music_loops_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/music_loops.bin)
endif()

if(BUILD_TESTING)
  add_executable(hmp_object_loops_test tests/hmp_object_loops_test.cpp)
  target_link_libraries(hmp_object_loops_test PRIVATE earthsiege_core)
  add_test(NAME hmp_object_loops COMMAND hmp_object_loops_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hmp_object_loops.bin)
endif()

if(BUILD_TESTING)
  add_executable(hdd_refresh_test tests/hdd_refresh_test.cpp)
  target_link_libraries(hdd_refresh_test PRIVATE earthsiege_core)
  add_test(NAME hdd_refresh COMMAND hdd_refresh_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hdd_refresh.bin)
  add_executable(cockpit_hdd_test tests/cockpit_hdd_test.cpp)
  target_link_libraries(cockpit_hdd_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_hdd COMMAND cockpit_hdd_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_hdd.bin)
  add_executable(tactical_targets_test tests/tactical_targets_test.cpp)
  target_link_libraries(tactical_targets_test PRIVATE earthsiege_core)
  add_test(NAME tactical_targets COMMAND tactical_targets_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tactical_targets.bin)
  add_executable(tactical_map_test tests/tactical_map_test.cpp)
  target_link_libraries(tactical_map_test PRIVATE earthsiege_core)
  add_test(NAME tactical_map COMMAND tactical_map_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tactical_map.bin)
endif()

if(BUILD_TESTING)
  add_executable(tactical_controls_test tests/tactical_controls_test.cpp)
  target_link_libraries(tactical_controls_test PRIVATE earthsiege_core)
  add_test(NAME tactical_controls COMMAND tactical_controls_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tactical_controls.bin)
endif()

if(BUILD_TESTING)
  add_executable(full_damage_test tests/full_damage_test.cpp)
  target_link_libraries(full_damage_test PRIVATE earthsiege_core)
  add_test(NAME full_damage COMMAND full_damage_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/full_damage.bin)
endif()

if(BUILD_TESTING)
  add_executable(career_test tests/career_test.cpp)
  target_link_libraries(career_test PRIVATE earthsiege_core)
  add_test(NAME career COMMAND career_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/career.bin ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign.bin)
endif()

if(BUILD_TESTING)
  add_executable(hangar_test tests/hangar_test.cpp)
  target_link_libraries(hangar_test PRIVATE earthsiege_core)
  add_test(NAME hangar COMMAND hangar_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hangar.bin)
endif()

if(BUILD_TESTING)
  add_executable(campaign_return_test tests/campaign_return_test.cpp)
  target_link_libraries(campaign_return_test PRIVATE earthsiege_core)
  add_test(NAME campaign_return COMMAND campaign_return_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_return.bin)
endif()

if(BUILD_TESTING)
  add_executable(launch_test tests/launch_test.cpp)
  target_link_libraries(launch_test PRIVATE earthsiege_core)
  add_test(NAME launch COMMAND launch_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/new_career.bin ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/launch.bin ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/launch_roster.bin)
endif()

if(BUILD_TESTING)
  add_executable(preferences_test tests/preferences_test.cpp)
  target_link_libraries(preferences_test PRIVATE earthsiege_core)
  add_test(NAME preferences COMMAND preferences_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/preferences.bin)
endif()

if(BUILD_TESTING)
  add_executable(results_test tests/results_test.cpp)
  target_link_libraries(results_test PRIVATE earthsiege_core)
  add_test(NAME results COMMAND results_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/results.bin)
endif()

if(BUILD_TESTING)
  add_executable(return_worlds_test tests/return_worlds_test.cpp)
  target_link_libraries(return_worlds_test PRIVATE earthsiege_core)
  add_test(NAME return_worlds COMMAND return_worlds_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/return_worlds.bin)
endif()

if(BUILD_TESTING)
  add_executable(armament_test tests/armament_test.cpp)
  target_link_libraries(armament_test PRIVATE earthsiege_core)
  add_test(NAME armament COMMAND armament_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/armament.bin)
endif()

if(BUILD_TESTING)
  add_executable(crew_assignment_test tests/crew_assignment_test.cpp)
  target_link_libraries(crew_assignment_test PRIVATE earthsiege_core)
  add_test(NAME crew_assignment COMMAND crew_assignment_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/crew_assignment.bin)
endif()

if(BUILD_TESTING)
  add_executable(component_repair_test tests/component_repair_test.cpp)
  target_link_libraries(component_repair_test PRIVATE earthsiege_core)
  add_test(NAME component_repair COMMAND component_repair_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/component_repair.bin)
endif()

if(BUILD_TESTING)
  add_executable(hangar_transactions_test tests/hangar_transactions_test.cpp)
  target_link_libraries(hangar_transactions_test PRIVATE earthsiege_core)
  add_test(NAME hangar_transactions COMMAND hangar_transactions_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hangar_transactions.bin)
endif()

if(BUILD_TESTING)
  add_executable(construction_menu_test tests/construction_menu_test.cpp)
  target_link_libraries(construction_menu_test PRIVATE earthsiege_core)
  add_test(NAME construction_menu COMMAND construction_menu_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/construction_menu.bin)
endif()

if(BUILD_TESTING)
  add_executable(launch_checks_test tests/launch_checks_test.cpp)
  target_link_libraries(launch_checks_test PRIVATE earthsiege_core)
  add_test(NAME launch_checks COMMAND launch_checks_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/launch_checks.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_animation_test tests/shell_animation_test.cpp)
  target_link_libraries(shell_animation_test PRIVATE earthsiege_core)
  add_test(NAME shell_animations COMMAND shell_animation_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_animations.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_animation_raster_test tests/shell_animation_raster_test.cpp)
  target_link_libraries(shell_animation_raster_test PRIVATE earthsiege_core)
  add_test(NAME shell_animation_rasters COMMAND shell_animation_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_animation_rasters.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_animation_sound_test tests/shell_animation_sound_test.cpp)
  target_link_libraries(shell_animation_sound_test PRIVATE earthsiege_core)
  add_test(NAME shell_animation_sound COMMAND shell_animation_sound_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_animation_sound.bin)
endif()

if(BUILD_TESTING)
  add_executable(hangar_damage_art_test tests/hangar_damage_art_test.cpp)
  target_link_libraries(hangar_damage_art_test PRIVATE earthsiege_core)
  add_test(NAME hangar_damage_art COMMAND hangar_damage_art_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hangar_damage_art.bin)
endif()

if(BUILD_TESTING)
  add_executable(bay_animation_plan_test tests/bay_animation_plan_test.cpp)
  target_link_libraries(bay_animation_plan_test PRIVATE earthsiege_core)
  add_test(NAME bay_animation_plan COMMAND bay_animation_plan_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bay_animation_plan.bin)
endif()

if(BUILD_TESTING)
  add_executable(bay_scene_test tests/bay_scene_test.cpp)
  target_link_libraries(bay_scene_test PRIVATE earthsiege_core)
  add_test(NAME bay_scene COMMAND bay_scene_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bay_scene.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_audio_test tests/shell_audio_test.cpp)
  target_link_libraries(shell_audio_test PRIVATE earthsiege_core)
  add_test(NAME shell_audio COMMAND shell_audio_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_music.bin)
endif()

if(BUILD_TESTING)
  add_executable(briefing_labels_test tests/briefing_labels_test.cpp)
  target_link_libraries(briefing_labels_test PRIVATE earthsiege_core)
  add_test(NAME briefing_labels COMMAND briefing_labels_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/briefing_labels.bin)

  add_test(NAME shell_bitmaps COMMAND bitmap_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_bitmaps.bin)

  add_executable(briefing_opening_test tests/briefing_opening_test.cpp)
  target_link_libraries(briefing_opening_test PRIVATE earthsiege_core)
  add_test(NAME briefing_opening COMMAND briefing_opening_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/briefing_opening.bin)

  add_executable(briefing_map_test tests/briefing_map_test.cpp)
  target_link_libraries(briefing_map_test PRIVATE earthsiege_core)
  add_test(NAME briefing_map COMMAND briefing_map_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/briefing_map.bin)
endif()

if(BUILD_TESTING)
  add_executable(briefing_tour_test tests/briefing_tour_test.cpp)
  target_link_libraries(briefing_tour_test PRIVATE earthsiege_core)
  add_test(NAME briefing_tour COMMAND briefing_tour_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/briefing_tour.bin)
endif()

if(BUILD_TESTING)
  add_executable(panorama_test tests/panorama_test.cpp)
  target_link_libraries(panorama_test PRIVATE earthsiege_core)
  add_test(NAME panorama COMMAND panorama_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/panorama.bin)
  add_executable(cockpit_panorama_test tests/cockpit_panorama_test.cpp)
  target_link_libraries(cockpit_panorama_test PRIVATE earthsiege_core)
  add_test(NAME cockpit_panorama COMMAND cockpit_panorama_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_panorama.bin)
endif()

if(BUILD_TESTING)
  add_executable(ground_marks_test tests/ground_marks_test.cpp)
  target_link_libraries(ground_marks_test PRIVATE earthsiege_core)
  add_test(NAME ground_marks COMMAND ground_marks_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ground_marks.bin)
  add_executable(clouds_test tests/clouds_test.cpp)
  target_link_libraries(clouds_test PRIVATE earthsiege_core)
  add_test(NAME clouds COMMAND clouds_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/clouds.bin)
endif()

if(BUILD_TESTING)
  add_executable(ground_texture_test tests/ground_texture_test.cpp)
  target_link_libraries(ground_texture_test PRIVATE earthsiege_core)
  add_test(NAME ground_texture COMMAND ground_texture_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ground_texture.bin)
  add_test(NAME ground_replay_precision COMMAND ground_texture_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/ground_replay_precision.bin")
endif()

if(BUILD_TESTING)
  add_executable(environment_palette_test tests/environment_palette_test.cpp)
  target_link_libraries(environment_palette_test PRIVATE earthsiege_core)
  add_test(NAME environment_palette COMMAND environment_palette_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/environment_palette.bin)
  add_executable(environment_composition_test tests/environment_composition_test.cpp)
  target_link_libraries(environment_composition_test PRIVATE earthsiege_core)
  add_test(NAME environment_composition COMMAND environment_composition_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/environment_composition.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_speech_test tests/shell_speech_test.cpp)
  target_link_libraries(shell_speech_test PRIVATE earthsiege_core)
  add_test(NAME shell_speech COMMAND shell_speech_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_speech.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_captions_test tests/shell_captions_test.cpp)
  target_link_libraries(shell_captions_test PRIVATE earthsiege_core)
  add_test(NAME shell_captions COMMAND shell_captions_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_captions.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_speech_selection_test tests/shell_speech_selection_test.cpp)
  target_link_libraries(shell_speech_selection_test PRIVATE earthsiege_core)
  add_test(NAME shell_speech_selection COMMAND shell_speech_selection_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_speech_selection.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_speech_scenes_test tests/shell_speech_scenes_test.cpp)
  target_link_libraries(shell_speech_scenes_test PRIVATE earthsiege_core)
  add_test(NAME shell_speech_scenes COMMAND shell_speech_scenes_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_speech_scenes.bin)
endif()

if(BUILD_TESTING)
  add_executable(game_shell_speech_test tests/game_shell_speech_test.cpp)
  target_link_libraries(game_shell_speech_test PRIVATE earthsiege_core)
  add_test(NAME game_shell_speech COMMAND game_shell_speech_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_TESTING)
  add_executable(shell_cinematics_test tests/shell_cinematics_test.cpp)
  target_link_libraries(shell_cinematics_test PRIVATE earthsiege_core)
  add_test(NAME shell_cinematics COMMAND shell_cinematics_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_cinematics.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_mechanic_test tests/shell_mechanic_test.cpp)
  target_link_libraries(shell_mechanic_test PRIVATE earthsiege_core)
  add_test(NAME shell_mechanic COMMAND shell_mechanic_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_mechanic.bin)
endif()

if(BUILD_TESTING)
  add_executable(game_shell_cinematics_test tests/game_shell_cinematics_test.cpp)
  target_link_libraries(game_shell_cinematics_test PRIVATE earthsiege_core)
  add_test(NAME game_shell_cinematics COMMAND game_shell_cinematics_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_TESTING)
add_executable(bay_speech_test tests/bay_speech_test.cpp)
target_link_libraries(bay_speech_test PRIVATE earthsiege_core)
add_test(NAME bay_speech COMMAND bay_speech_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bay_speech.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_lpf_test tests/shell_lpf_test.cpp)
  target_link_libraries(shell_lpf_test PRIVATE earthsiege_core)
  add_test(NAME shell_lpf COMMAND shell_lpf_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_lpf.bin)
endif()

if(BUILD_TESTING)
  add_test(NAME shell_credits COMMAND shell_cinematics_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_credits.bin)
endif()

if(BUILD_TESTING)
  add_executable(game_shell_doors_test tests/game_shell_doors_test.cpp)
  target_link_libraries(game_shell_doors_test PRIVATE earthsiege_core)
  add_test(NAME game_shell_doors COMMAND game_shell_doors_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_TESTING)
 add_executable(cockpit_impact_test tests/cockpit_impact_test.cpp)
 target_link_libraries(cockpit_impact_test PRIVATE earthsiege_core)
 add_test(NAME cockpit_impact COMMAND cockpit_impact_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_impact.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_statistics_test tests/shell_statistics_test.cpp)
  target_link_libraries(shell_statistics_test PRIVATE earthsiege_core)
  add_test(NAME shell_statistics COMMAND shell_statistics_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_statistics.bin)
endif()

if(BUILD_TESTING)
  add_executable(shell_single_return_test tests/shell_single_return_test.cpp)
  target_link_libraries(shell_single_return_test PRIVATE earthsiege_core)
  add_test(NAME shell_single_return COMMAND shell_single_return_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shell_single_return.bin)
endif()

if(BUILD_TESTING)
  add_executable(control_settings_test tests/control_settings_test.cpp)
  target_link_libraries(control_settings_test PRIVATE earthsiege_core)
  add_test(NAME control_settings COMMAND control_settings_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/control_settings.bin)
  add_executable(control_settings_raster_test tests/control_settings_raster_test.cpp)
  target_link_libraries(control_settings_raster_test PRIVATE earthsiege_core)
  add_test(NAME control_settings_raster COMMAND control_settings_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/control_settings_raster.bin)
endif()

if(BUILD_TESTING)
  add_executable(joystick_input_test tests/joystick_input_test.cpp)
  target_link_libraries(joystick_input_test PRIVATE earthsiege_core)
  add_test(NAME joystick_input COMMAND joystick_input_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/joystick_input.bin)
endif()

if(BUILD_TESTING)
  add_executable(joystick_calibration_test tests/joystick_calibration_test.cpp)
  target_link_libraries(joystick_calibration_test PRIVATE earthsiege_core)
  add_test(NAME joystick_calibration COMMAND joystick_calibration_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/joystick_calibration.bin)
endif()

if(BUILD_TESTING)
  add_executable(calibration_raster_test tests/calibration_raster_test.cpp)
  target_link_libraries(calibration_raster_test PRIVATE earthsiege_core)
  add_test(NAME calibration_raster COMMAND calibration_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/joystick_calibration_raster.bin)
endif()

if(BUILD_TESTING)
  add_executable(joystick_routing_test tests/joystick_routing_test.cpp)
  target_link_libraries(joystick_routing_test PRIVATE earthsiege_core)
  add_test(NAME joystick_routing COMMAND joystick_routing_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/joystick_routing.bin)
endif()

if(BUILD_TESTING)
  add_executable(joystick_devices_test tests/joystick_devices_test.cpp src/platform/joystick_devices.cpp)
  target_link_libraries(joystick_devices_test PRIVATE earthsiege_core glfw)
  add_test(NAME joystick_devices COMMAND joystick_devices_test)
endif()

if(BUILD_TESTING)
  add_executable(input_options_test tests/input_options_test.cpp)
  target_link_libraries(input_options_test PRIVATE earthsiege_core)
  add_test(NAME input_options COMMAND input_options_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/input_options.bin)
endif()

if(BUILD_TESTING)
 add_executable(game_shell_controls_test tests/game_shell_controls_test.cpp)
 target_link_libraries(game_shell_controls_test PRIVATE earthsiege_core)
 add_test(NAME game_shell_controls COMMAND game_shell_controls_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_TESTING)
 add_executable(frame_pacing_test tests/frame_pacing_test.cpp)
 target_link_libraries(frame_pacing_test PRIVATE earthsiege_core)
 add_test(NAME frame_pacing COMMAND frame_pacing_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frame_pacing.bin)
endif()

# Original complete recorded-input path, including post-read control policy.
target_sources(earthsiege_core PRIVATE src/simulation/tape_input.cpp)
if(BUILD_TESTING)
 add_executable(tape_input_test tests/tape_input_test.cpp)
 target_link_libraries(tape_input_test PRIVATE earthsiege_core)
 add_test(NAME tape_input COMMAND tape_input_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tape_packets.bin ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tape_routing_cases.bin)
endif()

# Original in-flight options, used by both live controls and recorded demos.
target_sources(earthsiege_core PRIVATE src/shell/mission_preferences.cpp src/render/mission_preferences_artwork.cpp)
if(BUILD_TESTING)
 add_executable(mission_preferences_test tests/mission_preferences_test.cpp)
 target_link_libraries(mission_preferences_test PRIVATE earthsiege_core)
 add_test(NAME mission_preferences COMMAND mission_preferences_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mission_preferences.bin)
 add_executable(mission_preferences_artwork_test tests/mission_preferences_artwork_test.cpp)
 target_link_libraries(mission_preferences_artwork_test PRIVATE earthsiege_core)
 add_test(NAME mission_preferences_artwork COMMAND mission_preferences_artwork_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mission_preferences_rasters.bin)
endif()

target_sources(earthsiege_core PRIVATE src/shell/mission_options_dialog.cpp)
if(BUILD_TESTING)
 add_test(NAME mission_controls_raster COMMAND control_settings_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mission_controls_rasters.bin flight)
 add_test(NAME mission_calibration_raster COMMAND calibration_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mission_calibration_rasters.bin flight)
endif()

target_sources(earthsiege_core PRIVATE src/render/shape_preferences.cpp)
if(BUILD_TESTING)
 add_executable(shape_preferences_test tests/shape_preferences_test.cpp)
 target_link_libraries(shape_preferences_test PRIVATE earthsiege_core)
 add_test(NAME shape_preferences COMMAND shape_preferences_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shape_preferences.bin)
 add_test(NAME shape_preferences_raster COMMAND actor_detail_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shape_preference_rasters.bin preferences)
endif()

if(BUILD_TESTING)
 add_executable(mission_options_integration_test tests/mission_options_integration_test.cpp)
 target_link_libraries(mission_options_integration_test PRIVATE earthsiege_core)
 add_test(NAME mission_options_integration COMMAND mission_options_integration_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/nested_flight_preferences.bin)
endif()

if(BUILD_TESTING)
 add_executable(alert_input_test tests/alert_input_test.cpp)
 target_link_libraries(alert_input_test PRIVATE earthsiege_core)
 add_test(NAME alert_input COMMAND alert_input_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/alert_input.bin)
endif()

if(BUILD_TESTING)
 add_test(NAME texture_narrow_quads COMMAND texture_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/texture_narrow_quads.bin)
 add_executable(audit_replay_raster_clocks tools/audit_replay_raster_clocks.cpp)
 target_link_libraries(audit_replay_raster_clocks PRIVATE earthsiege_core)
 add_executable(audit_demo_replay tools/audit_demo_replay.cpp)
 target_link_libraries(audit_demo_replay PRIVATE earthsiege_core)
 foreach(demo RANGE 1 4)
  add_test(NAME demo${demo}_replay COMMAND audit_demo_replay ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/demo${demo}_replay.bin)
 endforeach()
endif()

target_sources(earthsiege_core PRIVATE src/shell/cockpit_transition.cpp src/render/cockpit_pan.cpp)
if(BUILD_TESTING)
 add_executable(cockpit_transition_test tests/cockpit_transition_test.cpp)
 target_link_libraries(cockpit_transition_test PRIVATE earthsiege_core)
 add_test(NAME cockpit_transition COMMAND cockpit_transition_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_transition.bin)
endif()

if(BUILD_TESTING)
 add_executable(cockpit_pan_test tests/cockpit_pan_test.cpp)
 target_link_libraries(cockpit_pan_test PRIVATE earthsiege_core)
 add_test(NAME cockpit_pan COMMAND cockpit_pan_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_pan.bin ${CMAKE_CURRENT_SOURCE_DIR}/assets)
endif()

if(BUILD_TESTING)
 add_executable(cockpit_startup_test tests/cockpit_startup_test.cpp)
 target_link_libraries(cockpit_startup_test PRIVATE earthsiege_core)
 add_test(NAME cockpit_startup COMMAND cockpit_startup_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_startup.bin ${CMAKE_CURRENT_SOURCE_DIR}/assets)
endif()

if(BUILD_TESTING)
 add_executable(palette_device_test tests/palette_device_test.cpp)
 target_link_libraries(palette_device_test PRIVATE earthsiege_core)
 add_test(NAME palette_device COMMAND palette_device_test "${PROJECT_SOURCE_DIR}/tests/fixtures/palette_device.bin")
endif()

if(BUILD_TESTING)
  add_executable(software_cursor_test tests/software_cursor_test.cpp)
  target_link_libraries(software_cursor_test PRIVATE earthsiege_core)
  add_test(NAME software_cursor COMMAND software_cursor_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/software_cursors.bin")
endif()

if(BUILD_TESTING)
 add_executable(cockpit_edges_test tests/cockpit_edges_test.cpp)
 target_link_libraries(cockpit_edges_test PRIVATE earthsiege_core)
 add_test(NAME cockpit_edges COMMAND cockpit_edges_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cockpit_edges.bin)
endif()

if(BUILD_TESTING)
 add_executable(empty_portraits_test tests/empty_portraits_test.cpp)
 target_link_libraries(empty_portraits_test PRIVATE earthsiege_core)
 add_test(NAME empty_portraits COMMAND empty_portraits_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/empty_portraits.bin)
endif()

if(BUILD_TESTING)
 add_executable(external_view_test tests/external_view_test.cpp)
 target_link_libraries(external_view_test PRIVATE earthsiege_core)
 add_test(NAME external_view COMMAND external_view_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/external_view.bin)
endif()

if(BUILD_TESTING)
 add_test(NAME folded_texture COMMAND texture_raster_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/folded_texture.bin)
endif()

if(BUILD_TESTING)
 add_executable(weapon_message_erasure_test tests/weapon_message_erasure_test.cpp)
 target_link_libraries(weapon_message_erasure_test PRIVATE earthsiege_core)
 add_test(NAME weapon_message_erasure COMMAND weapon_message_erasure_test ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_message_erasure.bin)
endif()

if(BUILD_TESTING)
 add_executable(clock_retained_test tests/clock_retained_test.cpp)
 target_link_libraries(clock_retained_test PRIVATE earthsiege_core)
 add_test(NAME clock_retained COMMAND clock_retained_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/clock_retained.bin")
endif()

if(BUILD_TESTING)
  add_executable(midi_pipeline_test tests/midi_pipeline_test.cpp)
  target_link_libraries(midi_pipeline_test PRIVATE earthsiege_core)
  add_test(NAME midi_pipeline COMMAND midi_pipeline_test "${PROJECT_SOURCE_DIR}/tests/fixtures/midi_pipeline.bin")
endif()

if(BUILD_TESTING)
  add_executable(music_devices_test tests/music_devices_test.cpp)
  target_link_libraries(music_devices_test PRIVATE earthsiege_core)
  add_test(NAME music_devices COMMAND music_devices_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_BINARY_DIR}")
  set(EARTHSIEGE_TEST_ROM_DIR "" CACHE PATH "Optional matching Roland ROM sets for real hardware synthesis tests")
  set(EARTHSIEGE_TEST_SOUNDFONT "" CACHE FILEPATH "Optional General MIDI SoundFont for synthesis tests")
  if(EARTHSIEGE_TEST_ROM_DIR AND EARTHSIEGE_TEST_SOUNDFONT)
    add_test(NAME music_hardware COMMAND music_devices_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_BINARY_DIR}" "${EARTHSIEGE_TEST_ROM_DIR}" "${EARTHSIEGE_TEST_SOUNDFONT}")
  endif()
endif()

if(BUILD_TESTING)
  add_executable(platform_transport_test tests/platform_transport_test.cpp)
  target_link_libraries(platform_transport_test PRIVATE earthsiege_core Threads::Threads)
  add_test(NAME platform_transport COMMAND platform_transport_test)
  option(EARTHSIEGE_TEST_WINDOW "Test the real GLFW/OpenGL presenter (requires a display)" OFF)
  if(EARTHSIEGE_TEST_WINDOW)
    add_executable(platform_window_test tests/platform_window_test.cpp src/platform/window.cpp)
    target_link_libraries(platform_window_test PRIVATE earthsiege_core glfw OpenGL::GL)
    add_test(NAME platform_window COMMAND platform_window_test)
  endif()
endif()

include(cmake/install.cmake)
