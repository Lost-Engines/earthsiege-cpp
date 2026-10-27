cmake_minimum_required(VERSION 3.30)
project(earthsiege VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)
find_package(SDL2 REQUIRED CONFIG)

add_library(earthsiege_core
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
