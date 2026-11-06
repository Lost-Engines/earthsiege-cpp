# Expansion integration

The March 1995 CD expansion is bundled in `assets/expansion`. The graphical
launcher lets you select either edition. `--expansion` starts it directly;
`--data-dir PATH` also recognises an installed expansion by its patch volumes.

Expansion saves use the `expansion` subdirectory of the normal user save
location. An explicit `--save-dir` is used as supplied. The expansion shell can
select Battleset 1 or Battleset 2. The transfer screen reads original careers
from its own `car` directory, or from the neighbouring default base-game save
directory when that list is empty. Transferring a career from the latter creates
new expansion files and leaves the base game's saves unchanged.

## Implemented and checked

- Archive usage masks and priority preserve the original resource overrides.
- All 37 expansion mission sources are catalogued: 24 campaign missions,
  12 selectable single missions and the retained S2_RCN03 variant.
- 74 captured launches match the original SCRIPT.DAT, MISSION.VAR and
  PLAYER.MEC bytes. The expansion compiler counts unnamed templates when
  replacing source records and adding positions, but excludes them from exported
  references. This differs from the original game's compiler.
- Original constructed starting worlds cover 2,149 actors and 1,212 groups.
- 16 complete new-career constructors cover both battlesets, four difficulty
  levels and two RNG seeds. Eight transfers compare every career/history byte,
  hangar record and resulting RNG state, for both equipment choices.
- 2,200 technology checks cover mission milestones and results. Battleset 2
  uses three eight-mission chapters and its own equipment awards and restocking.
- Nine service-bay HERCs include Apocalypse, its nine mounts, damage diagrams,
  construction artwork, prices and worker animations. New weapon turn previews,
  labels and inventory entries are connected.
- All nine cockpit resources use the expansion's palette-copy rules. Wasteland
  and Ruins work in the mission renderer and briefing map.
- Weapon checks cover all expanded selectors, EMP-II's two partial-charge shots,
  the changed bullet lifetime clock, and 56 beam geometry/RNG/timer cases.
  108 bullet traces compare 4,794 native updates, including spread, damage
  falloff, plasma homing and proximity bursts. Recorded demos enable falloff
  independently of preference 17, matching the original.
- The landing craft has three weapon banks, each with a missile launcher and
  two beam weapons. 3,240 controller frames and 506 launches match original
  acquisition headings, timers, damaged-bank suppression, muzzle transforms
  and RNG. Its six weapon sections can be destroyed separately from the hull.
- Structure checks cover all eleven types, 2,112 collision rays and 1,628
  lifecycle steps. Expansion structures use an explicit collision flag and
  eight component slots. Ordinary expansion hits bypass the base edition's
  difficulty multiplier; plasma radial damage applies its separate multiplier.
  160 original radial shield cases cover front/rear absorption and integer limits.
- The two CD films decode directly from their original DFL streams. All 731
  indexed frames and display palettes match the independently executed original
  decoder. Audio begins on the authored frame; the shell uses 67 ms per frame.
  Installs without the films use the original TTM fallback presentations.
- A shell integration test draws all nine hangar models, fits all four upgraded
  weapons, launches and returns from single/campaign missions, and saves/resumes.
- Deployment modes 2-5 use the original approach angles, collision search and
  timing. 192 captured searches cover headings, seeds and rejected positions;
  24 pod sequences cover descent, opening, warning/impact sounds, obstruction
  and release. Landed pods leave the authored crater and use radial impact damage.
- 32 complete mission-return transactions compare saved careers, hangars,
  inventory, repair costs, debrief values and RNG. They cover both battlesets,
  crew losses, salvage and Apocalypse. Expansion salvage is restored to full
  condition; the three Cybrid-only weapon types cannot enter the armory.
- 432 environment comparisons cover all 36 WLD resources, cockpit masks and
  smaller offset viewports, checking every VGA pixel,
  reflection palette entry and shared visual RNG state. The expansion changes
  ground scan conversion, texture scale, sky-band edges and cloud placement.
  Wasteland/Ruins use a 6,000-unit cloud altitude rather than 10,000.
- All four expansion TAP demos run to the end with simulation, commands, cockpit
  dialogs, periodic rendering and audio updates. All 74 captured mission launches
  also construct, advance and render successfully.
- Expansion audio includes the deployment sounds, speech overrides and Battleset 2
  combat music. KEYJOY adds PEDALS reversal; 34 original parser cases cover all
  four switches, repeated settings and malformed input. GO saves 18 preference
  bytes and DBSIM saves 22; both are accepted.

The complete suite passes 294 tests with the local optional music assets. Eleven
expansion suites also pass ASan/UBSan and leak checking, including all four demos.

## Validation limits

The full expansion demo runs are integration checks. They do not yet compare
all actor state, every rendered frame or sound timing with an original replay.
The isolated native comparisons above establish the behaviour of their specific
systems. Full campaign playthroughs and consecutive whole-world expansion
comparisons remain useful follow-up coverage.

The implementation targets the March CD engine. The separate floppy simulator
and April patch have been characterised in the analysis, but are not selectable
runtime rulesets here. Exact base-game duplicates reuse their existing resources;
French/German media remains catalogued in the analysis while the bundled game
uses English.

The companion analysis catalogue is described in
`earthsiege-reverse/docs/EXPANSION_INTEGRATION.md`. Original-code probes and media
extraction tools stay in that research tree. C++ tests use compact captured
fixtures; the native game never runs the DOS executables.
