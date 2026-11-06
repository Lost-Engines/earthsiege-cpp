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
- The two CD films decode directly from their original DFL streams. All 731
  indexed frames and display palettes match the independently executed original
  decoder. Audio begins on the authored frame; the shell uses 67 ms per frame.
  Installs without the films use the original TTM fallback presentations.
- A shell integration test draws all nine hangar models, fits all four upgraded
  weapons, launches and returns from single/campaign missions, and saves/resumes.

## Work still in progress

The landing craft's component damage and weapons, deployment effects, plasma
homing and projectile falloff still require their own runtime comparisons.
Expansion TAP packets are decoded and checked, but complete recorded-gameplay
replay equivalence has not yet been established. An initial render or successful
mission launch does not settle those behaviours.

The companion analysis catalogue is described in
`earthsiege-reverse/docs/EXPANSION_INTEGRATION.md`. Original-code probes and media
extraction tools stay in that research tree. C++ tests use compact captured
fixtures; the native game never runs the DOS executables.
