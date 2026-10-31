# Current validation and fidelity limits

The native Earthsiege 1.0 game implements the original single-mission and career
loops: registration and saves, briefing and launch, HERC equipment and crew,
combat and objectives, return/debrief, salvage, repair, construction, progression,
cinematics, music, effects and speech. It loads the bundled original resources
directly; DOSBox and x86 emulation are only research tools.

This is the current baseline for comparison playtesting. The chronological
[implementation record](IMPLEMENTATION.md) preserves earlier milestones;
its old lists of unfinished integration are superseded by this page.

The current complete test suite passes **263/263 tests**. The figures below are
from the validated build: the existing 260 checks and all three new mission/campaign suites pass.

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
| Host integration | Private SDL-window checks cover pause freezing, preferences, status, cancellation and return to the shell, using isolated saves. |

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

Two reference boundaries are explicit:

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

Both are confirmed original memory defects. Formation zero is a chosen bounded
fallback, not recovered authorial intent. Repaired-reference comparisons are
not claims of byte-for-byte behavior after corruption in an untouched DOS run.

The full evidence, original addresses, capture paths and reproduction commands
are in [the analysis mission report](../../earthsiege-reverse/docs/MISSION_FIDELITY.md).
Fixture provenance records handoff and reference hashes, retained frame ranges
and each formation/queue repair. No audio waveform matching was added here;
the existing frame-boundary limit remains below.

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
- Original invalid reads/writes and uninitialized values have bounded handling
  where preserving memory corruption would be unsafe. The visible departures
  are listed in the [README](../README.md) and implementation record.
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
