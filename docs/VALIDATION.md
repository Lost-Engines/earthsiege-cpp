# Current validation and fidelity limits

The native Earthsiege 1.0 game implements the original single-mission and career
loops: registration and saves, briefing and launch, HERC equipment and crew,
combat and objectives, return/debrief, salvage, repair, construction, progression,
cinematics, music, effects and speech. It loads the bundled original resources
directly; DOSBox and x86 emulation are only research tools.

This is the current baseline for comparison playtesting. The chronological
[implementation record](IMPLEMENTATION.md) preserves earlier milestones;
its old lists of unfinished integration are superseded by this page.

The current complete test suite passes **260/260 tests**. The figures below are
from the validated build, including the retained clock pages, weapon/message overlaps and collapsed texture quads.

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
| Campaign/shell | Original launch files and RNG for 670 GO launches, 396 campaign transitions, four new-career constructors, and eighteen consecutive launch/return/repair transactions across all three battle sets. Separate original comparisons cover grading, recovery, inventories, crew, repairs and construction. |
| Graphics | Original model hierarchies, equipment, animation, damage, lighting, culling, scenery ordering, textures, palette effects and all 24 environments have component comparisons. Recent additions include 66 ground-projection cases, 3,000 clipped-line rasters and 94 missile-camera scenes with ground attachments. |
| Cockpit state | Original controls, view transitions, modal suspension, retained gauge pixels and separate map/damage timers are exercised. The timer fixture includes 274 sequential page-switch states. |
| Host integration | Private SDL-window checks cover pause freezing, preferences, status, cancellation and return to the shell, using isolated saves. |

The replay harness supplies recorded external inputs: input packets, observed
interface/radio clocks, installed preferences and original speech-driver
acceptance results. Component tests separately exercise the corresponding
native state machines. This isolates deterministic game logic from independent
hardware interrupt and voice-allocation phases.

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

- The four recordings and component/transaction sweeps do not constitute a
  manually completed playthrough of every campaign branch.
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
