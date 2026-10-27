cmake_minimum_required(VERSION 3.30)
project(earthsiege VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)
find_package(SDL2 REQUIRED CONFIG)

add_library(earthsiege_core
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
  src/simulation/mission_session.cpp
  src/simulation/mission_session.h
  src/resources/name_table.cpp
  src/resources/name_table.h
  src/simulation/camera.cpp
  src/simulation/camera.h
  src/simulation/arcsine_table.h
  src/simulation/keyboard_input.cpp
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
target_include_directories(earthsiege_core PUBLIC src "${PROJECT_SOURCE_DIR}")
target_compile_definitions(earthsiege_core PUBLIC VECTORSTORM_NO_BOOST)
target_compile_options(earthsiege_core PUBLIC
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
)

add_executable(earthsiege
  src/main.cpp
  src/platform/window.cpp
  src/platform/window.h
)
target_link_libraries(earthsiege PRIVATE earthsiege_core SDL2::SDL2)

if(BUILD_TESTING)
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
  add_executable(projectile_flight_test tests/projectile_flight_test.cpp)
  target_link_libraries(projectile_flight_test PRIVATE earthsiege_core)
  add_test(NAME projectile_flight COMMAND projectile_flight_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/projectile_flight.bin")
  add_executable(weapon_controls_test tests/weapon_controls_test.cpp)
  target_link_libraries(weapon_controls_test PRIVATE earthsiege_core)
  add_test(NAME weapon_controls COMMAND weapon_controls_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_controls.bin")
  add_executable(damage_test tests/damage_test.cpp)
  target_link_libraries(damage_test PRIVATE earthsiege_core)
  add_test(NAME damage COMMAND damage_test "${PROJECT_SOURCE_DIR}/tests/fixtures/damage.bin")
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
  add_executable(line_raster_test tests/line_raster_test.cpp)
  target_link_libraries(line_raster_test PRIVATE earthsiege_core)
  add_test(NAME line_raster COMMAND line_raster_test "${PROJECT_SOURCE_DIR}/tests/fixtures/line_raster.bin")
  add_executable(model_renderer_test tests/model_renderer_test.cpp)
  target_link_libraries(model_renderer_test PRIVATE earthsiege_core)
  add_test(NAME model_renderer COMMAND model_renderer_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/fixtures/model_render.bin")
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
