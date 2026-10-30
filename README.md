# Earthsiege C++

A native C++26 reconstruction of **Metaltech: Earthsiege**, beginning with the original 1994 version 1.0. Development is in progress. Single missions and careers can now be launched from the menus, flown and returned to the shell with saved results. Opening, travel and ending cinematics are connected; remaining presentation timing is still being verified. This is not yet a finished game.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/earthsiege
```

Requires a C++26 compiler, CMake 3.30 or newer and SDL2 development files. Original freeware game resources are bundled in `assets`; `--data-dir PATH` selects another installation. The floating window uses square pixels and the largest integer scale that fits the desktop. No DOS executable or emulator is needed by the native program.

`--inspect-demo 1` (or 2–4) opens a recorded mission’s starting terrain and HERC roster. Use arrows/keypad to walk and turn, I/J/K/M or right mouse to aim, and Space to fire. W/Alt+W cycles weapons; Tab cycles targets; Enter confirms tactical-panel commands. F1–F6 select the six MFD pages. Ctrl+F1 shows the cockpit, Ctrl+F2 orbits and Ctrl+F3 detaches. F7/F8 select the lower tactical map/damage panel and F9/F10 the left/right panels. Ctrl + arrows controls the external camera; P pauses and Escape returns to the menu. The cockpit throttle and weapon charge sliders, shield allocation buttons, weapon groups, LINK and TRACK controls respond to the mouse. Click a numbered weapon to select it, right-click to change its group membership, or use 1–9. The HUD shows heading, torso and pitch indicators, target lock brackets and the next waypoint. The cockpit clock and animated lamps use the original presentation scripts.

The lower panel shows the original tactical map, wingmate orders and structural,
internal and weapon damage diagrams. On the map, 1–3 select a wingmate; the marked
command keys select orders, clicking the map chooses a destination, Tab cycles
eligible contacts, X transmits and Backspace cancels. Double-clicking an already
selected command also transmits when it is ready. The arrow buttons pan or change
the inspected damage subject/page; +/− controls map scale. S/I/W select the three
damage pages. Damage labels use the actual pilot's name.

The MFD displays live status, communications, navigation, radar, target condition and missile-camera views. FLASH COMM sends real wingmate orders; Alt+A/G/H/O/C/E/F provide the original attack, ignore, help, form-up, scan, EMCON and fire/hold shortcuts. Radio acknowledgements now display their original text, pilot portraits and static transitions. Their selection, scheduling, pixels and mouth timing match original executable captures; speech output is connected. MFD damage now uses the original flicker timing and OFFLINE label.

This inspection mode uses mission AI, combat and objectives. It does not replay the recorded inputs. Normal missions are selected from the main menu; Q opens the original mission-status/return dialog, P pauses and Ctrl+Escape or Ctrl+Q asks to quit.

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

Campaign saves preserve the original 41-state career format, ordered equipment inventories and variable hangar records. They pass complete original-executable save/read comparisons, all 396 campaign filename/branch transitions and 140 grade thresholds. Registration, selection, resume and manual backups use these original formats. Saves live in the platform user-data directory (normally `~/.local/share/lost-engines/earthsiege` on Linux); `--save-dir PATH` overrides that location. The bundled resources and comparison DOS installation are not modified.

The portable shell now also implements mission-return salvage, ordered equipment awards/restocking, pilot statistics and replacements, technology unlocks, repair budgets and chassis construction. Complete original return/save comparisons cover 96 scenarios (including 144 replacement pilots); repair checks cover 6,400 quotes, 1,280 transactions and 25 construction states. The arming screen supports original clickable mounts, compatible weapon lists, condition-preserving equipment exchange, missile types and HERC selection. Crew and repair screens are connected; construction controls are now connected too. The service bay now animates its original workers, elevators and welding, including partial-construction artwork. Menu music, workshop sounds and the original weapon turn previews are connected. Briefing maps now include the original guided tour, zoom/pan and object callouts. Commander briefings, chapter introductions, departure remarks and single-mission debriefs now include their original voices, captions and animated portraits. Opening, chapter-travel and ending cinematics are connected, along with campaign mechanic response chains, menu doors and credits. --skip-intro skips the opening and pilot-death movie. Remaining presentation timing is still being verified.

Launch preparation now matches all 670 retained original GO launches byte for byte (SCRIPT.DAT, MISSION.VAR and PLAYER.MEC), including exact random-generator state. Four complete new-career constructors and 32 additional roster exports cover history initialization, inactive crew and differing pilot health. The shell's original default missile-type normalization and selected-pilot health export behaviour are retained.

Mission result export now retains recovered weapons and crew damage for the shell. All 5,114 captured recovery/condition states and four complete starting-world handoffs match. The original writer reads beyond its nine equipment slots for a tenth unused condition; this implementation safely writes 100 for that unused word.

The nineteen mission alerts match captured DOS pixels. Eighteen consecutive
launch/reload/return/repair/next-mission transactions also match the original
across all three battle sets. Each flight return restores the saved inventory
and starts the shell's time-seeded RNG again, as the separate DOS executable
did. Manual career backups remain explicit actions. These comparisons verify
the handoff and persistence, not an entire interactively completed campaign.

Arming comparisons cover 1,650 equipment exchanges, 360 mount menus, 270 missile
selections and 128 HERC-selection cases. The original bay artwork and mount
positions are used. Samson grenade-launcher entries refer to missing bay images
in the supplied data; those layers are omitted safely while keeping the equipment.

Crew Assignment now uses the original portraits, wireframe HERC diagrams and
screen palette. Reassignment preserves the original temporary swap behaviour;
Cancel restores all four assignments. Pilot registration starts with an empty
name field. Repair allocation and policy controls are connected. Construction uses the original blueprint artwork and unlocked chassis selection.

Detailed repairs retain the original condition bands, refunds, cancellation and
Proceed behaviour, including its extra uncharged condition step. Scrap returns
usable equipment to inventory and credits damaged equipment and chassis salvage.
The original repair code corrupts crew-header fields when repairing an unassigned
HERC; this implementation performs the repair without that invalid pilot-share
write. The native comparison covers 5,323 component edits, eight constructions
and 128 scrap/cancel transactions.

Construction menu comparisons cover another 1,424 original events. Unfinished
builds persist across restarts and finish after the original number of mission
returns. The original post-cancellation affordability omission is retained: after
scrapping a partial build, immediately rebuilding can overdraw the material
balance. Empty reserved bays are valid and do not advance construction.

The flight environment now includes the original sky bands, horizon panorama,
projected ground texture, moving clouds, procedural ground marks and
pitch-dependent reflective palette. Combined output matches 288 original
frames across all 24 worlds, including cockpit masks and detail settings.
Live frame scheduling, impact flashes and cockpit displacement are integrated.

The original cinematic fade-in reads an uninitialized rectangle type. This build initializes it, giving deterministic fades instead of depending on prior stack contents.

Cockpit damage now flashes the original damage palette and displaces the cockpit and instruments vertically while the outside projection stays steady. The original timing, random sequence and view-change behaviour are retained.

Options / Controls now supports the original joystick profiles, pedals, mouse setting and calibration screens. Tab and Alt+Tab move focus; right-click cycles an option backwards. The first two connected SDL controllers supply the original joystick axes, with a single four-axis stick supported as an alternative. Calibrations are saved automatically beside the user saves. Insert or joystick button 2 swaps movement/turret control, button 3 selects a target and button 4 stops movement or centres the turret. Hats follow the original cockpit-view rules.

The bundled DATA/KEYJOY.CFG retains the original pitch/turn choices. Malformed tokens are reported safely instead of reproducing the original out-of-bounds table read. SDL calibration uses separate host-joy files so it cannot be confused with DOS gameport counts. See docs/IMPLEMENTATION.md for the host axis and throttle mappings.

Side and lower cockpit views now scroll between panels using the original pixel steps. Music and sound continue during the scroll.

Live missions now use the original elapsed-time pacing and bounded simulation
steps. Audio continues during mission dialogs; losing window focus suspends it.


F12 or Alt+P opens the original in-flight preferences panel, with a rotating HERC preview. Music, effects, radio mode, horizon, sky, ground and model detail update live. Controls and joystick calibration are available inside it; Escape accepts a panel and returns to its parent. Closing preferences restores the previous flight view.


Cockpit edge strips switch between forward, lower and side views, with the
original directional pointers. Pilot selection uses the authored indicator
lamps. The original unused-portrait clearing quirk is reproduced without its
invalid stack pointer.


External views retain the original camera distance and rotation between switches,
with the original framing and status labels. Hidden clouds pause while the lower
cockpit panel is open. Complete demo checks now include camera state.
If a distorted texture polygon requests pixels outside its image, those pixels
are safely omitted rather than reading unrelated memory.
