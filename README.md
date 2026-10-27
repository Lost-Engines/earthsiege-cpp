# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress; this first commit provides checked loading of all original resources. It is not yet a playable game.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. No DOS executable or emulator is needed by the native program.

Code is licensed under GPLv3 or later. Original game assets remain the property of their respective owners and are separate from the code licence. Reverse-engineering notes and comparison tools live in the sibling `earthsiege-reverse` project.
