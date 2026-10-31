include(FetchContent)
find_package(Threads REQUIRED)
set(libmt32emu_SHARED OFF CACHE BOOL "Link Munt into the game")
FetchContent_Declare(munt
  URL https://codeload.github.com/munt/munt/tar.gz/5ee078a761d35cdd3d27000b551c12d11df6a60b
  URL_HASH SHA256=f2c0885a6a9b40329019424afddf800d53b4a85b2f8d23658a246d2883201b0c
  SOURCE_SUBDIR mt32emu
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(munt)

FetchContent_Declare(nuked_sc55
  URL https://codeload.github.com/jcmoyer/Nuked-SC55/tar.gz/f3464753f64a7da5f5fd3a96fd72197628369589
  URL_HASH SHA256=241cf3ebc1022ce5a519d42b6c318e3c8a8359994e7598227d5e4af050542a76
  SOURCE_SUBDIR unused_upstream_build
)
FetchContent_MakeAvailable(nuked_sc55)

FetchContent_Declare(tinysoundfont
  URL https://codeload.github.com/schellingb/TinySoundFont/tar.gz/853a0a171759f1ddba0de1442133a75912bbeffa
  URL_HASH SHA256=76c356df524f71f2a34f67fa0cb2cb946036fe7a65d2930a849569c5e742fa57
  SOURCE_SUBDIR unused_upstream_build
)

FetchContent_MakeAvailable(tinysoundfont)
include(cmake/sc55.cmake)
include(cmake/tinysoundfont.cmake)
