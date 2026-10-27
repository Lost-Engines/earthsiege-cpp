# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress; the main menu, native scene renderer and HERC movement are working. Full missions are not yet playable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. The floating window uses square pixels and the largest integer scale that fits the desktop. No DOS executable or emulator is needed by the native program.

`--inspect-demo 1` (or 2–4) opens a recorded mission’s starting terrain and player roster. Use arrows/keypad to walk and turn, I/J/K/M to aim, Ctrl + arrows to move the camera, and Ctrl + Shift + up/down to adjust distance. F1 attaches the camera, F2 orbits, F3 detaches, P pauses and Escape returns to the menu. This is an inspection mode: combat, collision response, AI and cockpit are still being integrated. It does not replay the recorded inputs.

For automated comparisons, `--headless --screenshot frame.ppm` saves a native 320×200 frame; `--frames N` bounds an interactive run.

Code is licensed under GPLv3 or later. Original game assets remain the property of their respective owners and are separate from the code licence. Reverse-engineering notes and comparison tools live in the sibling `earthsiege-reverse` project.
