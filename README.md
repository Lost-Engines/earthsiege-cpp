# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress; the main menu, native scene renderer, HERC movement and firing are working. Full missions are not yet playable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. The floating window uses square pixels and the largest integer scale that fits the desktop. No DOS executable or emulator is needed by the native program.

`--inspect-demo 1` (or 2–4) opens a recorded mission’s starting terrain and HERC roster. Use arrows/keypad to walk and turn, I/J/K/M or right mouse to aim, and Space to fire. W/Alt+W cycles weapons; Enter/Tab cycles targets. F1–F6 select the six MFD pages. Ctrl+F1 shows the cockpit, Ctrl+F2 orbits and Ctrl+F3 detaches. F7/F8 select the lower/forward panels and F9/F10 the left/right panels. Ctrl + arrows controls the external camera; P pauses and Escape returns to the menu. The cockpit throttle and weapon charge sliders, shield allocation buttons, weapon groups, LINK and TRACK controls respond to the mouse. Click a numbered weapon to select it, right-click to change its group membership, or use 1–9. The HUD shows heading, torso and pitch indicators, target lock brackets and the next waypoint. The cockpit clock and animated lamps use the original presentation scripts.

The MFD displays live status, communications, navigation, radar, target condition and missile-camera views. FLASH COMM sends real wingmate orders; Alt+A/G/H/O/C/E/F provide the original attack, ignore, help, form-up, scan, EMCON and fire/hold shortcuts. Radio acknowledgements now display their original text, pilot portraits and static transitions. Their selection, scheduling, pixels and mouth timing match original executable captures; speech output is still being integrated. MFD damage now uses the original flicker timing and OFFLINE label.

This is an inspection mode with mission AI, combat and objectives. Cockpit instruments and the campaign interface are still being integrated. It does not replay the recorded inputs.

The original group dispatcher can read an uninitialized priority word for commands such as form up. This reconstruction uses neutral priority for those commands, making delivery deterministic. Radio queues start with zeroed unused indices. Original commander-message loading writes outside its descriptor table; this reconstruction retains the messages as resources and preserves their observed absence from radio output, without copying the memory corruption. The death-report portrait uses a zero offset, matching the captured original allocation; its offset is not initialized by the original asset loader.

For automated comparisons, `--headless --screenshot frame.ppm` saves a native 320×200 frame; `--frames N` bounds an interactive run.

Code is licensed under GPLv3 or later. Original game assets remain the property of their respective owners and are separate from the code licence. Reverse-engineering notes and comparison tools live in the sibling `earthsiege-reverse` project.

PCM playback retains the original 8-bit mixer’s gain steps, saturation and
buffered completion. The original positional-volume query divides by zero if
queried while the master gain is zero; this reconstruction returns zero in that
case. Unused, manually supplied sample-loop ranges are bounded to their data.

Mission inspection now plays the original PCM effects and wingmate speech through SDL audio. Firing, explosions, radar switches, flyer engines and authored player footsteps use their original triggers and distance/pan rules. Aiming servos, target-lock tones and the cockpit engine loop are connected too; music is still being integrated.
