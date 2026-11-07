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
set instead. The opening cinematics and edition selector are skipped, and a key
or mouse click returns to the main menu, as does the end of playback.

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
