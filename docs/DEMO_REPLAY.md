# Recorded-input demo verification

A demo contains starting mission files and timed inputs. It does not contain a
player path. Every collision, weapon discharge, AI decision and input transition
must run through the game again when it plays.

The expansion Apocalypse patrol on the red world is DEMO3, mission B1_C2_0A.
Its first building collision exposed a missing throttle-ownership flag. Movement
stopped the HERC, but the cockpit restored the old throttle on its next update.
The corrected exchange accepts the collision stop, allowing the recorded reverse
movement and detour to work.

## Selecting a recording

Launch `earthsiege --demo 1` through `--demo 4` to play a specific original-game
recording. Add `--expansion` for the expansion recordings; its Apocalypse patrol
is `earthsiege --expansion --demo 3`. An explicit `--data-dir` selects that data
set instead. Playback starts immediately. Press Escape to return to the main
menu, or let the recording finish. Clicking the window and switching away with
Alt+Tab both leave playback running.

The native View Demo menu chooses `1 + (current time in seconds % 4)`. It is a
clock-based choice, not a sequence through the recordings. `--demo` overrides
only the initial playback; later menu selections use the usual clock choice.
`--frames` also bounds playback, and `--screenshot` saves the last displayed
frame when that limit is reached. `--inspect-demo` remains a separate live
inspection mode and cannot be combined with `--demo`.

The DOS simulator also accepts named playback. From the installed game directory,
with the usual CD mount and `ALT_DRIVE` environment in place, run:

```dos
DBSIM -eggplant -pTAPES\DEMO3
```

Use the expansion's installation for its recordings. The name is attached to
lowercase `-p`, without a space or `.TAP` extension. This bypasses the shell and
runs the simulator directly; it is the command used for the original-game
comparison captures. DOS View Demo itself has no numbered picker. Its `-D`
selector reads `TAPES/DEMOLIST.STR` and uses the current time modulo the number
of entries, retrying if it matches the selector's retained previous index.

## Recording a demo

From the sibling `earthsiege-reverse` checkout, run
`python3 tools/record_demo.py 2` to make separate DOSBox and C++ recordings
of retail DEMO2. Add `--source cpp` to record only this version or
`--edition expansion` to select the expansion tapes. The files are written
under `work/videos` in the analysis checkout.

The harness builds the optional `capture_demo` target. It runs the same
replay as View Demo, samples the 320 by 200 framebuffer and its palette at
70 Hz, and writes stereo samples from the game mixer. It includes the
intermediate frames during cockpit pans and the clean page shown between
mouse cursor flashes. The tape clock sets the timing, so recording can run
with the game window closed.

## How fast should a demo play?

A normal world step in both DOS executables is 12 timer ticks, about 100 ms.
In live play DBSIM measures the time since the previous step and scales the
movement by that interval. A fast machine can run more than ten updates per
second. At the minimum wait of two ticks it advances the world by three
ticks' worth of movement. Tying that loop to a modern display can make the
HERC walk up to 50% too fast.

The native game schedules a full world step every 100 ms. It carries spare
timer ticks into the next step when a display refresh arrives late. A long
stall, such as a cockpit pan, gets one update at the original upper limit.
Input, window presentation and the audio mixer run on their own schedules.

For demos, the recording supplies the pace. Just before a world update the
writer saves the integer part of `elapsed_ticks * 256 / 12`, clipped to
64 through 450. An interior value of 213, for example, comes from exactly
ten timer ticks. After drawing a frame the player can recover how long to
hold it by reading the value in the *next* world packet. It carries fractional
milliseconds between frames. The original movement code still receives the
stored value on each update.

| Recording and interval | Time on the recording clock | DOSBox at 60,000 cycles |
| --- | ---: | ---: |
| Retail DEMO3, world packets 6-55 | 3.083 s | 2.154 s |
| Retail DEMO3, world packets 201-300 | 9.042 s | 5.251 s |
| Retail DEMO3, exit-dialog mouse gesture | 0.800 s | 0.010 s |

The mouse supplies an independent check. In retail DEMO1 the timer accounts
for 27.225 seconds between packets 1476 and 1802. The last mouse event in
each packet puts them 27.233 seconds apart. In expansion DEMO3 the same check
gives 25.092 and 25.100 seconds between packets 2989 and 3216. Mouse events
can arrive between two world updates. Packet numbers here start at one.

The lift has no tape packets. A value of 64 could mean two or three ticks.
At 450 only the first 22 ticks are certain. During a dialog the world is
frozen and DBSIM writes the last movement value on every poll. The player
uses three ticks at the lower limit. At the upper limit it allows for any
cockpit scroll and time already spent in a dialog. Where a dialog contains
two timestamped mouse batches, it spreads their elapsed time over the polls
between them.

For intervals the recording leaves open, the eight files in `assets/demo-clocks`
start with DOSBox measurements for the lift, upper-clipped waits, untimed
dialog polls and the exit. For 335 upper-clipped steps the mouse timestamps
also constrain the total time between two ordinary world polls. Where the
DOSBox estimate ended too early to allow the recorded mouse movement, the
extra ticks are spread across the clipped steps in that stretch. Their
individual durations remain estimates. In expansion DEMO4 the mouse covers
332.183 seconds between packets 629 and 3304. The player takes 332.199
seconds across those 253 clipped steps and the ordinary steps between them.

Each file is named for the TAP's CRC32. Its first little-endian
16-bit word counts the startup intervals, followed by those intervals and one
interval per input packet, all in milliseconds. A replacement tape also uses
its own world intervals and mouse timestamps. Without a matching capture,
its lift uses the native timer, an untimed alert polls immediately, and the
moving options preview allows one VGA refresh per poll.

The DOS replay branch bypasses the live timer. At 60,000 cycles retail DEMO3
took 4.601 seconds for the lift and 91.923 seconds from the first tape input
to exit. Those numbers describe playback in that emulator. Adding the
movement values from all 2,470 polls gives 204.77 seconds because that
calculation counts frozen dialogs as world updates.

In expansion DEMO3 a hit shakes the lower map panel a few pixels into the
forward view while the player presses `1` to select a plasma weapon. DOS
sends number keys to the active cockpit view, so the visible edge of the map
cannot intercept that selection.

## The cursor over the gunsight

Retail DEMO3 opens with a small green diamond over the gunsight. After the
pilot looks to the side and back, a white cross flashes between the green
arms. Moving the mouse in live play separates the two. The arms belong to
the HUD. The cross is `CURSOR.DCI`, a seven-by-seven sprite with 13 visible
pixels and its hotspot in the middle.

DOS saves the pixels under the cursor so it can restore them when it flips
the VGA page. In the cockpit the clean image is visible for about one 70 Hz
refresh. The cursor then appears on the next page. Its visible time varies
with the length of the world step. Replay draws the cursor at the recorded
mouse position, so moving the host mouse while watching a demo leaves it
on its recorded path.

## Original comparison

The analysis project runs the unchanged March 1995 expansion executable and TAP
files under a private DOSBox observer. The observer reads state without patching
the game. C++ uses `demo_playback`, the same owner as Options / View Demo, with
matching maximum-detail preferences. The native game receives recorded inputs,
not expected positions, damage or random results.

Each checked-in fixture stores the original player position, three angles,
throttle, gait, steering and animation rate after every simulation update. It
also stores the complete gameplay RNG before each update. This boundary matters:
the original end hook precedes command dispatch, while the native step includes
it. Comparing RNG at the next update's beginning gives both versions the same
boundary.

| Expansion recording | Mission | Simulation updates | Input packets | Modal packets |
|---|---|---:|---:|---:|
| DEMO1 | B1_C1_1A | 2,226 | 2,274 | 48 |
| DEMO2 | B1_C2_2A | 1,554 | 1,588 | 34 |
| DEMO3 | B1_C2_0A | 3,455 | 3,605 | 150 |
| DEMO4 | B1_C1_2A | 3,836 | 3,954 | 118 |
| Total | | 11,071 | 11,421 | 350 |

All four comparisons pass with exact integer equality at every checked update.

## Corrections made during the audit

Following the first differing update exposed several expansion rules that had
been using base-game behaviour:

- Tracking servos stop at the requested angle, wingmates avoid the player's
  firing line, and guard movement continues between enemy searches.
- Obstacle avoidance counts as attacking for target ranking and retaliation.
  DEFEND can follow a friendly HERC rather than a fixed map position. WEAPONS
  FREE measures the distance to the guarded objective when deciding whether to
  pursue an enemy.
- Flyers compose pitch and bank matrices before applying world yaw. Their pitch
  steering and previous-yaw state also differ from the base engine.
- Buildings update after projectiles. Passive buildings are excluded from
  target selection and hostile-hit contact sharing.
- Shield recharge, collision damage, projectile splash, upgraded laser pulses,
  EMP charging and campaign difficulty follow their expansion routines.
- The easiest expansion campaign setting makes the player invulnerable. Later
  shots from a dead attacker use its current state when deciding retaliation.
- The tactical map uses the expansion projection and mission-sized zoom/pan
  range. Cockpit radio shortcuts are ignored in external views.
- Escape returns from external views, keypad 5 stops movement, grave cycles
  weapon groups, and shield shortcuts apply in recorded command order.

Independent original-instruction fixtures additionally cover 6,000 tracking
controls, 1,000 shield recharges, 1,600 firing-line avoidance cases, 2,000 damage
accumulators, 2,000 EMP recharge cases, 512 map pans, 20 controller definitions,
24 invulnerability settings, 1,024 guard decisions and 2,304 flyer movement steps.
The analysis project's `docs/DEMO_REPLAY.md` records addresses and the path plot.

## Scope and remaining differences

These fixtures check player motion and gameplay RNG throughout the recordings.
They do not establish equality of every rendered pixel, audio clock or internal
actor field, nor do four recordings cover every campaign interaction.

The existing FORM UP fix remains intentional. The original group dispatcher
reads an uninitialised priority for that command. In DEMO3 it reads -8193 and
silently skips delivery at update 3144, then reads -1 and succeeds at 3240. The
native neutral priority delivers the first order. Companion 20 consequently
changes route from update 3212, while player motion remains identical.

The broader trace also retains two small differences for further investigation:
DEMO1 companion 17 differs by one heading unit for two updates before converging,
and DEMO4's player turret centres one update earlier, converging after five
updates. Neither changes the checked player path or gameplay RNG. The original
trace and diagnostic C++ tool retain these fields so they can be investigated
without relying on visual recollection.

## Running the checks

```sh
ctest --test-dir build -R '^expansion_demo[1-4]$' --output-on-failure
build/trace_demo_path assets/expansion 3 build/demo3-path.jsonl
```

`trace_demo_path` records player and actor motion, aiming, power, damage,
awareness, controllers and gameplay RNG. Its output is a diagnostic artifact;
only compact original outputs and their provenance belong in `tests/fixtures`.
