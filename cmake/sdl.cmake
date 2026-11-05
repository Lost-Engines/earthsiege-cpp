option(EARTHSIEGE_BUNDLED_SDL "Build a pinned, static SDL2 for portable packages" OFF)
if(EARTHSIEGE_BUNDLED_SDL)
  include(FetchContent)
  if(APPLE)
    enable_language(OBJC)
  endif()
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(sdl2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
    URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(sdl2)
else()
  find_package(SDL2 2.0.18 REQUIRED CONFIG)
endif()
