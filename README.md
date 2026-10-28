# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress; the main menu, native scene renderer, HERC movement and firing are working. Full missions are not yet playable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. The floating window uses square pixels and the largest integer scale that fits the desktop. No DOS executable or emulator is needed by the native program.

`--inspect-demo 1` (or 2–4) opens a recorded mission’s starting terrain and HERC roster. Use arrows/keypad to walk and turn, I/J/K/M or right mouse to aim, and Space to fire. W/Alt+W cycles weapons; Enter/Tab cycles targets. F1 shows the cockpit, F2 orbits and F3 detaches. F7/F8 select the lower/forward panels and F9/F10 the left/right panels. Ctrl + arrows controls the external camera; P pauses and Escape returns to the menu. The cockpit throttle and weapon charge sliders, shield allocation buttons, weapon groups, LINK and TRACK controls respond to the mouse. Click a numbered weapon to select it, right-click to change its group membership, or use 1–9. The HUD shows heading, torso and pitch indicators, target lock brackets and the next waypoint. The cockpit clock and animated lamps use the original presentation scripts.

This is an inspection mode with mission AI, combat and objectives. Cockpit instruments and the campaign interface are still being integrated. It does not replay the recorded inputs.

For automated comparisons, `--headless --screenshot frame.ppm` saves a native 320×200 frame; `--frames N` bounds an interactive run.

Code is licensed under GPLv3 or later. Original game assets remain the property of their respective owners and are separate from the code licence. Reverse-engineering notes and comparison tools live in the sibling `earthsiege-reverse` project.
