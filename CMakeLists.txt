cmake_minimum_required(VERSION 3.30)
project(earthsiege VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)
find_package(SDL2 REQUIRED CONFIG)

add_library(earthsiege_core
  src/resources/archive.cpp
  src/resources/archive.h
  src/resources/binary_reader.cpp
  src/resources/binary_reader.h
  src/resources/resource_store.cpp
  src/resources/resource_store.h
)
target_include_directories(earthsiege_core PUBLIC src)
target_compile_options(earthsiege_core PUBLIC
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
)

add_executable(earthsiege src/main.cpp)
target_link_libraries(earthsiege PRIVATE earthsiege_core SDL2::SDL2)

if(BUILD_TESTING)
  add_executable(resource_test tests/resource_test.cpp)
  target_link_libraries(resource_test PRIVATE earthsiege_core)
  add_test(NAME resources COMMAND resource_test "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_SOURCE_DIR}/tests/resource_checks.tsv")
endif()
