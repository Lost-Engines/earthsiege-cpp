# Explicit Runtime contents keep local saves, ROMs and research out of packages.
install(TARGETS earthsiege RUNTIME DESTINATION . COMPONENT Runtime)
install(FILES README.md THIRD_PARTY.md DESTINATION . COMPONENT Runtime)
install(FILES docs/MUSIC.md docs/EXPANSION.md docs/VALIDATION.md docs/IMPLEMENTATION.md docs/PLATFORM.md docs/build-release.md
  DESTINATION docs COMPONENT Runtime)
install(FILES assets/README.md DESTINATION assets COMPONENT Runtime)
install(FILES assets/DATA/KEYJOY.CFG DESTINATION assets/DATA COMPONENT Runtime)
install(FILES assets/TAPES/DEMO1.TAP assets/TAPES/DEMO2.TAP
  assets/TAPES/DEMO3.TAP assets/TAPES/DEMO4.TAP
  DESTINATION assets/TAPES COMPONENT Runtime)
install(FILES assets/VOL/DETECT.VOL assets/VOL/SHELL0.VOL assets/VOL/SIMSOUND.VOL
  assets/VOL/SIMVOICE.VOL assets/VOL/SIMVOL0.VOL assets/VOL/SPEECH.VOL assets/VOL/ZONES.VOL
  DESTINATION assets/VOL COMPONENT Runtime)
install(DIRECTORY assets/expansion/ DESTINATION assets/expansion COMPONENT Runtime
  FILES_MATCHING PATTERN "*.VOL" PATTERN "*.TAP" PATTERN "*.CFG" PATTERN "*.DFL" PATTERN "*.RAW" PATTERN "README.md" PATTERN "SHA256SUMS")
install(FILES scripts/roms.txt DESTINATION scripts COMPONENT Runtime)
if(WIN32)
  install(PROGRAMS scripts/fetch-assets.bat DESTINATION scripts COMPONENT Runtime)
  install(CODE [[
    foreach(name IN ITEMS fetch-assets.bat roms.txt)
      set(path "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/scripts/${name}")
      file(READ "${path}" contents)
      string(REPLACE "\r\n" "\n" contents "${contents}")
      string(REPLACE "\n" "\r\n" contents "${contents}")
      file(WRITE "${path}" "${contents}")
    endforeach()
  ]] COMPONENT Runtime)
else()
  install(PROGRAMS scripts/fetch-assets.sh DESTINATION scripts COMPONENT Runtime)
endif()
install(DIRECTORY third_party/licenses/ DESTINATION third_party/licenses COMPONENT Runtime)
install(FILES third_party/nuked-opl3/LICENSE
  DESTINATION third_party/nuked-opl3 COMPONENT Runtime)
if(DEFINED glfw_SOURCE_DIR)
  install(FILES "${glfw_SOURCE_DIR}/LICENSE.md"
    DESTINATION third_party/licenses RENAME GLFW-zlib.txt COMPONENT Runtime)
endif()
install(FILES "${miniaudio_SOURCE_DIR}/LICENSE"
  DESTINATION third_party/licenses RENAME miniaudio.txt COMPONENT Runtime)
