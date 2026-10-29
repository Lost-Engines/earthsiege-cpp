# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress; the main menu, native scene renderer, HERC movement and firing are working. Full missions are not yet playable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. The floating window uses square pixels and the largest integer scale that fits the desktop. No DOS executable or emulator is needed by the native program.

`--inspect-demo 1` (or 2–4) opens a recorded mission’s starting terrain and HERC roster. Use arrows/keypad to walk and turn, I/J/K/M or right mouse to aim, and Space to fire. W/Alt+W cycles weapons; Enter/Tab cycles targets. F1–F6 select the six MFD pages. Ctrl+F1 shows the cockpit, Ctrl+F2 orbits and Ctrl+F3 detaches. F7/F8 select the lower tactical map/damage panel and F9/F10 the left/right panels. Ctrl + arrows controls the external camera; P pauses and Escape returns to the menu. The cockpit throttle and weapon charge sliders, shield allocation buttons, weapon groups, LINK and TRACK controls respond to the mouse. Click a numbered weapon to select it, right-click to change its group membership, or use 1–9. The HUD shows heading, torso and pitch indicators, target lock brackets and the next waypoint. The cockpit clock and animated lamps use the original presentation scripts.

The lower panel shows the original tactical map, wingmate orders and structural,
internal and weapon damage diagrams. On the map, 1–3 select a wingmate; the marked
command keys select orders, clicking the map chooses a destination, Tab cycles
eligible contacts, X transmits and Backspace cancels. Double-clicking an already
selected command also transmits when it is ready. The arrow buttons pan or change
the inspected damage subject/page; +/− controls map scale. S/I/W select the three
damage pages. Damage labels use the actual pilot's name.

The MFD displays live status, communications, navigation, radar, target condition and missile-camera views. FLASH COMM sends real wingmate orders; Alt+A/G/H/O/C/E/F provide the original attack, ignore, help, form-up, scan, EMCON and fire/hold shortcuts. Radio acknowledgements now display their original text, pilot portraits and static transitions. Their selection, scheduling, pixels and mouth timing match original executable captures; speech output is connected. MFD damage now uses the original flicker timing and OFFLINE label.

This is an inspection mode with mission AI, combat and objectives. Cockpit instruments and the campaign interface are still being integrated. It does not replay the recorded inputs.

The original group dispatcher can read an uninitialized priority word for commands such as form up. This reconstruction uses neutral priority for those commands, making delivery deterministic. Radio queues start with zeroed unused indices. Original commander-message loading writes outside its descriptor table; this reconstruction retains the messages as resources and preserves their observed absence from radio output, without copying the memory corruption. The death-report portrait uses a zero offset, matching the captured original allocation; its offset is not initialized by the original asset loader.

For automated comparisons, `--headless --screenshot frame.ppm` saves a native 320×200 frame; `--frames N` bounds an interactive run.

Code is licensed under GPLv3 or later. Original game assets remain the property of their respective owners and are separate from the code licence. Reverse-engineering notes and comparison tools live in the sibling `earthsiege-reverse` project.

PCM playback retains the original 8-bit mixer’s gain steps, saturation and
buffered completion. The original positional-volume query divides by zero if
queried while the master gain is zero; this reconstruction returns zero in that
case. Unused, manually supplied sample-loop ranges are bounded to their data.

Mission inspection now plays the original PCM effects and wingmate speech through SDL audio. Firing, explosions, radar switches, flyer engines and authored player footsteps use their original triggers and distance/pan rules. Aiming servos, target-lock tones and the cockpit engine loop are connected too. Music uses the original FM instruments and sampled percussion, with the original encounter-driven track selection and fades. OPL3 is currently the live default; both OPL2 and OPL3 driver paths have original-code comparisons.

FM synthesis uses the unmodified LGPL Nuked OPL3 core in `third_party/nuked-opl3`.
The host stream runs at 44.1 kHz; original 11.025 kHz PCM values are held between
DAC updates. FM and PCM receive equal mix headroom. This reproduces the software
protocol, not a particular Sound Blaster card’s analogue circuitry or balance.

Damage now produces attached smoke and physically ejected weapons, with the original animation timing, attachment transforms and debris motion.

The original lower-panel weapon labels retain shared text references for empty
mounts. After inspecting another HERC, those empty rows can display stale names.
This reconstruction preserves that visible quirk with bounded strings. The map
also preserves the original distinction between mouse selection range and Tab's
remembered-contact selection, and retains unmodified fields in outgoing orders.

Campaign/save foundations now preserve the original 41-state career format, ordered equipment inventories and variable hangar records. They pass complete original-executable save/read comparisons, all 396 campaign filename/branch transitions and 140 grade thresholds. These components are being connected to the interactive shell; ordinary campaign play is not yet available.
