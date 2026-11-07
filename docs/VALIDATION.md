# Current validation and fidelity limits

The native Earthsiege 1.0 game implements the original single-mission and career
loops: registration and saves, briefing and launch, HERC equipment and crew,
combat and objectives, return/debrief, salvage, repair, construction, progression,
cinematics, music, effects and speech. It loads the bundled original resources
directly; DOSBox and x86 emulation are only research tools.

This is the current baseline for comparison playtesting. The chronological
[implementation record](IMPLEMENTATION.md) preserves earlier milestones;
its old lists of unfinished integration are superseded by this page.

The last full expansion integration run passed **306/306 tests**, including the original-game
regressions, GLFW window check and the locally enabled firmware/SoundFont test.
Without optional hardware assets the suite has 305 tests. See [PLATFORM.md](PLATFORM.md) for the host integration checks.
The earlier 263-test baseline passed ASan/UBSan. After the final gameplay fixes, all 19
affected movement/world suites passed both sanitizers. The six audio suites,
including the new physical-MIDI captures and real synthesis backends, also pass
ASan/UBSan with leak checking after this audio update. The earlier expansion baseline passed eleven
ASan/UBSan suites with leak checking: all four demos, shell, mission returns,
deployment, landing craft, bullets, environments and input parsing. Those
sanitizer runs predate the latest recorded-demo audit. The 306-test result above
checks the current code in the normal local build.

## Options menu playtesting

The detail selector now follows GO's separate four-state preset. Initialization
uses a horizon contribution of 63 or 191, rather than adding the Boolean horizon
flag directly. Each click advances the retained preset and writes the original
63/127/191/255 detail values. Cancel restores both the label and the values.

View Demo now launches the recorded-input simulator from the shell. Playback
owns recorded modal input as well as ordinary frames, and returns to the main
menu without importing a demo result into a career. The expansion passes its
selected battleset to the simulator. Live keypresses or clicks end playback.

The `--demo N` launcher was checked locally for all eight recordings, rendering
120 frames and saving a distinct screenshot from each. Invalid numbers and
conflicting inspection options fail before window creation. Default edition,
`--demo=N`, explicit expansion data directories, and the two existing demo/menu
regressions also pass. These launch checks do not replace the full replay
comparisons below.

Two short `options_demo` regression tests cover both editions, the actual mouse
button handlers, all presets, Cancel, persistence, demo startup and interruption.
Full recording runs use `options_demo_test assets build full` and the equivalent
`assets/expansion` command locally. The ordinary shell-flow test also passes.
All four base and expansion recordings complete through this owner. Expansion
playback now compares the original player's coordinates, angles and movement
controls after every update, plus the complete gameplay RNG at each update's
entry. All 11,071 states match, consuming 11,421 packets with 350 modal polls.
This fixes the former premature DEMO4 alert and the Apocalypse patrol's failure
to reverse away from its first building. See [Recorded-input demo verification](DEMO_REPLAY.md)
for the original traces, recovered expansion rules and remaining actor-state
limitations.

The GLFW window was checked locally against the original DOSBox preferences:
Max displays correctly, all four presets cycle, a demo plays, and Escape returns
to the main menu.

## Weapon-link shortcut

L now reaches the cockpit weapon-link handler. It toggles the selected matching
pair before selection and firing, as in the original `0x380c4` branch, and plays
the same interface click. The existing `weapon_groups` test now also checks
linking and unlinking through a live mission, ignored Alt/Ctrl modifiers, and
the lower-panel/external-view restrictions. It and the original player-armament
comparisons pass locally.

## Keyboard and squad-view audit

The host adapter now includes F11, N, keypad Enter, keypad 0/Insert and all eight movement-key
aliases. Alt state updates on modifier-only events as well as held-control events.
The F11 objectives window matches six original full-frame captures exactly.
Squad-camera tests cover all four demos in both editions, including the fresh
orbit on first selection and restoration when returning to the player. An
original-code capture independently checks six switches, camera retention and
control ownership. [Keyboard coverage](KEYBOARD.md) distinguishes these fixes
from the original development and recording controls. Those controls now have
8,760 captured arithmetic/handler cases, 64 expansion pod destinations and
live mission routing checks in both editions, including Alt+S phase ordering.

The subsequent manual comparison found a separate WCS input gap: expansion
Ctrl+number presets are consumed by an earlier keyboard callback, before global
dispatch. The port's modern-axis route does not reproduce that callback and
its throttle feedback calculation. The 306-test baseline does not validate
that path. See [keyboard coverage](KEYBOARD.md#manual-comparison-and-remaining-wcs-input-coverage).

## Edition coverage

The comparisons below concern the original October 1994 version 1.0 build.
The supplied 1996 base reissue contains the same game files. The March 1995
expansion has separate [integration checks](EXPANSION.md), including original
mission launches, technology, careers, transfer, returns, weapons, landing craft,
deployment, input, 432 complete environment frames and CD films. All four
expansion demos match the original player motion and gameplay RNG throughout.
Complete expansion campaign playthroughs, full actor-state comparisons and
replay rendering/audio comparisons remain further coverage.

## Independent original comparisons

Expected outputs come from the original executable or unchanged DOSBox runs.
Native simulation and rendering do not consume expected positions, pixels,
damage, objectives or random-generator results.

| Coverage | Verified output |
|---|---|
| Four complete supplied recordings | All 9,300 input records consumed; 7,032 captured simulation/exit states, complete gameplay and visual RNG, and camera state agree. |
| Mission return | Final RESULTS.DAT and MISSION.VAR agree byte for byte in all four recordings. |
| Replay cockpit pictures | 584 weapon-label samples, 579 instrument regions/colour samples, twelve complete frames and six scene regions match. |
| Replay sound requests | All 7,073 ordered non-engine starts and sound stops agree. Engine starts/retries have the timing boundary below. |
| Campaign/shell | Original launch files and RNG for 670 GO launches, 396 campaign transitions, four new-career constructors, eighteen short-cycle transactions and 121 further consecutive returns through six victory/defeat journeys across all three battle sets. Separate original comparisons cover grading, recovery, inventories, crew, repairs and construction. |
| Graphics | Original model hierarchies, equipment, animation, damage, lighting, culling, scenery ordering, textures, palette effects and all 24 environments have component comparisons. Recent additions include 66 ground-projection cases, 3,000 clipped-line rasters and 94 missile-camera scenes with ground attachments. |
| Cockpit state | Original controls, view transitions, modal suspension, retained gauge pixels and separate map/damage timers are exercised. The timer fixture includes 274 sequential page-switch states. |
| Host integration | Private GLFW-window checks cover pause freezing, preferences, status, cancellation and return to the shell, using isolated saves. |

The replay harness supplies recorded external inputs: input packets, observed
interface/radio clocks, installed preferences and original speech-driver
acceptance results. Component tests separately exercise the corresponding
native state machines. This isolates deterministic game logic from independent
hardware interrupt and voice-allocation phases.

## Authored missions and campaign journeys

The authored-world comparison preserves the actual GO handoff files, group
activation, side assignments and initial roster. It does not force every group
active as the earlier controlled navigation/combat probes did. A fixed ratio
of 256, neutral player input and simulator RNG warm-up 17 make the runs repeatable.

All **75 supported mission sources** match for **256 consecutive frames** each
(19,200 frames). A further **ten later-career launches** match for 256 frames each
(2,560 frames), using actual next-launch files from the transaction journeys.
This is 21,760 additional world-state comparisons. The source sweep chooses one
seed-17 context per source, not every one of the 670 retained launch variations.
The two incompatible legacy mission formats remain outside the supported set.

For checked-in regression, mission_authored_frames.bin retains the first 64
consecutive frames of every source (4,800 frames); mission_career_frames.bin
retains all 2,560 career frames, including the effect-allocation check after
frame 203. Full 256-frame source captures remain in the analysis tree. Keeping
a prefix reduces fixture size; it does not omit intervening simulation steps.

Four additional seed-42 launches combine authored activation and factions with
player movement, aiming, firing, weapon selection, linking and target exchange.
All 1,024 frames match at ratio 128 and simulation RNG warm-up 83, and are retained
in mission_authored_player_frames.bin. ESWFRM07 carries both the three original
launch files and the recorded player-control inputs.

The longer player-controlled captures also cover loss of mobility. Original
13250 composes the evaluated cockpit transform with actor placement, extracts
its heading, recentres the torso and reevaluates the pose when the player's legs
fail. The native session now performs that transition before the remaining
frame work. Six previously differing 512-frame captures match after this fix,
including subsequent formation movement and collision damage. Three complete
cases (B0_C2_20, B0_C7_6A and SM_SCT02, 1,536 frames) are retained in
mission_player_disabled_frames.bin with original capture hashes.

A later-career reverse-playback case also exercises the one-unit animation
advance inside that disable routine. At cached rate -155, signed Q8 scaling
advances by -1; treating it as a pose-only refresh changed the final position
by two units. mission_disabled_reverse_frames.bin retains all 1,024 frames of
the corrected comparison.

The second source sweep exposed a ramming-damage ordering detail. Original
18278/1f75c pass the rammer's position by address. Self-damage can remove the
rammer during the victim loop and move that same centre underground. Native
code now retains the live reference, preventing damage at the copied old
position. mission_ramming_frames.bin retains all 512 B0_C5_50 frames, including
the affected armour sections and subsequent random state.

An extended sweep covers every supported source for 512 player-controlled
frames, with launch seed 42, simulator warm-up 83 and frame ratio 450: 38,400
frames, 752,640 HERC/flyer observations and 353,280 ground-object observations.
All cases agree after the native disable fix and the explicitly corrected
original references described below. Seventy-two cases need neither of the new
aim/arithmetic repairs; two need aim initialization and one needs the multiply
repair. Raw failures and the consolidated reviewed report remain in the analysis
tree. Three further neutral-input missions match for 2,048 frames each, adding
6,144 longer-running world frames.

A second 75-source sweep uses launch seed 17, simulator warm-up 131 and ratio
128, matching another 38,400 player-controlled frames. Ten actual later-career
handoffs run for 1,024 frames each at ratio 256 and warm-up 151, matching all
10,240 frames. Those runs include carried-over equipment, damage and mission
flags. Their references explicitly initialize the unwritten aim component and
repair the multiply helper. The original failure reports remain available;
final rechecks agree on 75/75, 75/75 and 10/10 cases respectively. Together
with the pilot and long runs, this pass adds 94,208 compared world frames.

The comparison covers controllers, movement, damage, power, sensors, animation,
target selection, ground objects, activation, route cursors, all 100 mission
counters, outcomes and the complete shared random state. Native output is never
used to generate the expected state.

The full progression journeys use a fresh original GO image on every return,
preserving only serialized state. Their six paths contain 40/15, 26/7 and 26/7
success/failure returns in battle sets 0, 1 and 2, ending at nodes 99 and -1.
All 121 transactions match career/hangar bytes, repair results, graph positions,
return and launch RNG, terminal readiness and the next three handoff files.
Grades, salvage, damage and statistics are controlled flight-result inputs.
These establish persistence/progression, not completed combat playthroughs.

Those journeys exposed a missing native selection step: campaign slots may name
BST random-mission lists. The shell now resolves them before compiling the chosen
mission, with the original one bounded RNG draw and duplicate-list weighting.
Mission simulations separately exposed constructor damage/controller ordering,
overbroad activation gating for ground objects, and uninstantiated source-target
mapping. Those behaviors now follow the original instructions.

The following original defects have explicit handling:

- Negative formation IDs make original follower calculations read before the
  allocation. Live native sessions choose formation zero. Strict inspection
  placement still represents the result as undefined. Reference probes record
  the same argument repair without altering the supplied handoff bytes.
- Original deferred effect queues store only 20 pointers before overwriting their
  own count at +80. B0_C5_70 retires 40 explosions and 40 secondary flat objects
  together. Native queues already support all objects within the original pool
  limits. The original probe extends pending storage, then runs the original FIFO
  destructor/free-list collector in batches at the same phase. Without that repair,
  corruption changes later allocation and random-number consumption.

- Original HERC construction leaves aimed-component word +0x238 unwritten.
  A travelling actor can aim before strategy assigns it. Native state initializes
  it to -1 for centre aiming. Two distinct sentinels survive original construction
  unchanged, and a write trace confirms the field remains unwritten before its
  first affected use. Initializing only that word makes the complete B0_C1_60 and
  B0_C4_00 captures agree. Both 512-frame cases are retained with explicit repair
  records in mission_initialized_aim_frames.bin.
- Long obstacle detours expose a fixed-point multiply bug in original 63d8c/5e838.
  IMUL's overflow carry changes the rounding constant through SBB ECX,ECX;
  the following ADD/ADC adds a spurious carry. At B0_C5_70 frame 354 this adds
  262,143 world units to the offset's X coordinate. Native arithmetic already
  uses a wide product and keeps that correct result. Replacing just that SBB
  with XOR in the reference emulator removes the divergence for all 512 frames.
  mission_corrected_offset_frames.bin records the two-byte reference patch.

Formation zero and centre aiming are chosen deterministic fallbacks. Original
allocation memory does not establish an intended value for either. The detour
calculation keeps correct Q14 rounding instead of reproducing the overflow bug.
The installed DOS executable is unchanged; corrected references are labelled
separately from captures of the untouched original.

The full evidence, original addresses, capture paths and reproduction commands
are in [the analysis mission report](../../earthsiege-reverse/docs/MISSION_FIDELITY.md).
Fixture provenance records handoff and reference hashes, retained frame ranges
and each formation, queue, initialization or arithmetic repair. No audio waveform matching was added here;
the existing frame-boundary limit remains below. The [extended analysis report](../../earthsiege-reverse/docs/AUTONOMOUS_VALIDATION.md) includes the instruction traces and arithmetic/constructor probes.

## Measured full-frame differences

The four unchanged original recordings have 120 sampled complete-frame
comparisons. After isolating presentation timing, **114 images are pixel
identical**. The other six differ only at the malformed distant radar structures:
**230 pixels total**, all explained by the existing bounded repair.

| Recording | Sampled frames | Exact images | Remaining differences |
|---|---:|---:|---|
| DEMO1 | 33 | 30 | 36, 33 and 30 pixels in frames 120, 1020 and 1440 |
| DEMO2 | 31 | 29 | 46 and 54 pixels in frames 660 and 720 |
| DEMO3 | 29 | 29 | None |
| DEMO4 | 27 | 26 | 31 pixels in frame 0 |

The radar models refer to nonexistent pose matrices. Original execution reads
the following allocation header and animation bytes as transforms; changing
only an allocator pointer changes visible pixels. Native loading maps those
seven invalid references to local partition planes. In a separate controlled
probe, applying only that same repair to the original renderer makes all six
remaining scenes agree, at every affected pixel and with no other pixel changes.
That modified probe is distinguished from the unchanged primary reference.

This is a confirmed original out-of-bounds read, and the repair is intentionally
retained in the authentic build. It is a narrow exception to the usual policy
of leaving bug fixes to Redux: reproducing this defect would require additional
machinery for the relevant DOS allocation layout and history. Local-space
partition planes are the chosen deterministic repair; the evidence establishes
the original defect, not the artists' intended replacement. The six affected
images remain documented departures from untouched DOS output.

The timing-controlled comparison supplies original clock/HMI elapsed inputs,
drawing-page context and VGA copy operations to the native clock and TTM
renderers. It asserts 7,024 clock states and pre-dispatch TTM states, rather than
assigning them. The native MAP refresh receives 975 actual timer polls, while
its deadlines and blink state remain native calculations. A separate 1,024-case
original-code fixture verifies the production clock's retained-page scheduling.
The native simulation and renderers never receive expected image pixels.

The original cursor is removed using its saved-under buffer. No rectangle is
masked out of the comparison. Before timing alignment, maximum full-frame
differences are 42/62/24/43 pixels for DEMO1/2/3/4; these raw reports are retained.
Different host interrupt/audio phases are still not promised to synchronize
naturally with a particular DOS run.

The analysis tree contains the complete
[pixel investigation](../../earthsiege-reverse/docs/PIXEL_FIDELITY.md),
the unchanged captures under analysis/tapes/replays/demoN-clock-pages, raw and
controlled reports under image-differences, and the six-scene repair probe.
This closes the identified differences in these samples; it does not establish
pixel identity for every possible game state.

## Explicit limits

- The four recordings, authored startup simulations and controlled terminal
  campaign journeys do not constitute a manually completed playthrough of every
  campaign branch, random layout or late combat interaction.
- Whole audio waveforms are not claimed identical. Engine samples 16/45 can
  retry on different frames as PCM completion and finite voice allocation cross
  a simulation-frame boundary. Engine stops and the other ordered requests are
  compared. The original drivers and synthesizer paths have separate tests.
- Host output does not reproduce a particular sound card's analogue circuitry.
- Original invalid reads/writes and uninitialized values have bounded handling.
  The long-detour multiply also keeps correct wide arithmetic. These departures
  are listed in the [README](../README.md) and explained above.
- Legacy resource fields without an established consumer retain neutral names.
  The analysis completion audit records those endpoints rather than assigning
  speculative meanings.

## Reproduce the checks

Run from the repository root:

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
~~~

For one complete replay and its diagnostic frames:

~~~sh
EARTHSIEGE_REPLAY_DIAGNOSTICS=build/replay-demo1 build/audit_demo_replay assets tests/fixtures/demo1_replay.bin
~~~

The reference-capture and image-comparison tools live in the sibling
earthsiege-reverse/tools directory. Regenerating expected fixtures requires
the original executable; running the C++ tests does not.

## Music hardware update

The new `midi_pipeline` fixture compares all twenty supplied HMP resources with
both original HMI physical-MIDI drivers. Across 722 scheduler/control steps per
case, all 30,764 output bytes and 3,092 sampled-percussion events match. The
reference runs original driver installation, routing, gain, stop cleanup and
MPU port writes; host inputs supply ready ports and explicit PCM completions.
Existing FM, gain, looping, shell-audio and mission-audio fixtures still pass.

The original MT-32 route does not select any supplied melodic track. This is
preserved separately from the optional GM-to-MT-32 listening experiment. The
original MT-32 master-gain mask is `0x02fe`; MPU-401 updates all sixteen channels.
See [music hardware](MUSIC.md) for the source-data limitations and user controls.

`music_devices` checks settings round trips, quoted paths, explicit-directory
precedence, both FM modes and failed-device-change recovery, including choices
made before the first song starts. `music_hardware`
adds real CM-32L, SC-55 and SCC-1A ROMs plus a SoundFont, note output, the menu
song with digital percussion, pause/resume and device changes. Configure the
optional test using paths to your local assets:

```sh
cmake -S . -B build \
  -DEARTHSIEGE_TEST_ROM_DIR=/path/to/roms \
  -DEARTHSIEGE_TEST_SOUNDFONT=/path/to/bank.sf2
ctest --test-dir build -R 'music_devices|music_hardware|midi_pipeline' --output-on-failure
```

The complete 272-test run passed. All six selected audio suites
passed ASan/UBSan with leak checking. Firmware and SoundFonts
are local test inputs, not checked-in fixtures. The captured original MIDI and
PCM protocol is checked on builds without those optional assets as well.


Before the platform migration, a private SDL/Xvfb check selected SC-55 through Ctrl+Shift+M, found ROMs without
an explicit path, saved the choice, restored it after restarting and resumed the
shell after Cancel without changing the configuration. Headless mission starts
also passed with SC-55 and the Munt MPU-401 mode. The Linux fetch helper completed
a fresh twelve-file download and verified every manifest hash. The Windows
helper follows the same manifest; it has not had a native Windows run.
