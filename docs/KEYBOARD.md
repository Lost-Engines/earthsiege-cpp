# Keyboard audit

The GLFW adapter now maps all fourteen original held controls, including the
navigation-key aliases for the numeric keypad. Commands use the original DOS
scan values, with Alt (`0x200`) and Ctrl (`0x400`) retained. Shift is handled by
text entry and discarded by the simulator's command filter.

The November 2026 audit compared the adapter with the original global, cockpit,
HERC and weapon dispatch tables, the held-key table, and the shortcut fields in
both editions' shell resources. The shell uses the same thirteen distinct
shortcut scans in both editions; all have host mappings.

## Fixed during the audit

| Key | Behaviour |
|---|---|
| F11 | Show the current mission's objectives while in the cockpit. Escape, Enter, keypad Enter or the Return button dismisses the window. |
| N | Cycle through squad HERCs while in external view. Each HERC retains its own orbit. A newly viewed HERC starts with the original zero-angle orbit. |
| Home / keypad 7 | Forward and left. |
| Page Up / keypad 9 | Forward and right; also the same page command in panels. |
| End / keypad 1 | Reverse and left. |
| Page Down / keypad 3 | Reverse and right; also the same page command in panels. |
| Keypad 0 | Same command as Insert, including joystick-mode switching and WCS dialog navigation. |
| Keypad Enter | Same command and held-button behaviour as Enter. |
| Alt | Changes aiming speed immediately, including when pressed or released after the aiming key. |

N only selects members of the player's group whose Z position is nonnegative,
as in DBSIM `0x59ed8`. Camera binding (`0x58a44`, `0x5923c`, `0x3d0`) retains
previously visited cameras and clears orbit controls on a new binding. Looking
at a wingmate always gives the keys to the camera. Tab/Enter retains the player's
camera/HERC-control choice; returning to the player restores that choice.
V or Escape returns to the player's cockpit.

F11 uses the objective text supplied by the current mission briefing or embedded
in the demo recording. The original dialog (`0x4a1b4`, `0x4a5a4`) displays the
first seven nonempty strings among at most twenty entries. The implementation
uses its HST_ALRT artwork, OBJ_ALRT labels and original fonts. Six full 320x200
reference frames cover empty objectives, blank lines, overflow and the pressed
Return button; every pixel matches. Existing alert input rules handle dismissal.

Local tests cover host aliases and modifier changes, the original F11 pixels,
and squad-camera selection/restoration across all four demos in both editions.
The older input fixtures still cover 4,096 held-key combinations and 3,000
press/release transitions. L's weapon-link routing was fixed in the preceding
playtesting update and has its own live-mission test.

## Original development and recording controls

These handlers are now implemented. They use the original modifier combinations
and remain separate from the ordinary movement keys.

| Key | Base game | Expansion |
|---|---|---|
| Alt+arrows / keypad 2, 4, 6, 8 | Reposition the viewed HERC along its local axes. | Same. |
| Ctrl+Left/Right / keypad 4, 6 | Turn the viewed HERC by the selected angular step. | Same. |
| Alt+Ctrl+1 through 9 | Select a translation/rotation step. | Same. |
| Ctrl+1 | Alternate forward-only and reverse-only throttle regimes; assigned to the shipped WCS reverse button. | Global handler is a no-op. The earlier documented WCS preset path is separate and remains incomplete in this port; see below. |
| Alt+Ctrl+P | Apply the original nearby-force damage calculation to the player. | Spawn an unassigned drop pod 50,000 units ahead of the player. |
| Alt+S | Freeze or resume world updates during tape playback. | Same. |

The nine steps are 500, 1000, 1500, 2000, 3000, 4500, 6000, 7500 and 9000.
Both steps default to 2000. Translation uses world units; rotation uses 65,536
units per turn. Position changes use the full local pitch/roll/heading transform
and bypass gait movement for that command. Normal simulation resumes afterwards.
When N selects a squadmate, the direct position/rotation keys affect that HERC.

Ctrl+1 initially selects forward-only, then reverse-only, alternating thereafter.
The initial unrestricted mode permits throttle from -1024 to +1024; forward-only
clamps it to 0..1024, and reverse-only negates the input and clamps to -1024..0.
Crossing neutral still stops at zero. Ctrl+1 is not an invulnerability command.

The base damage command first checks whether the player is under threat. It
counts eligible HERCs within 45,000 units using their profile combat ratings,
flyers within 50,000 units at 500 each, and armed structures within 35,000 units
at 500 each. It subtracts 600 from friendly strength, floors that at zero, then
applies `(64,64)` random-damage calls while enemy strength exceeds it, removing
400 from the assessed enemy strength per call. The normal invulnerability option
does not intercept this direct damage path. In the expansion, the global key
handler instead creates a drop pod with no assigned HERC or group. It still
falls, warns, impacts and opens, but releases no HERC and leaves no crater.

Alt+S is ignored in ordinary missions. During playback it freezes actor, effect,
projectile, AI and power updates. Tape packets, player input, camera handling,
area checks and mission-status handling continue. The command is processed
between the world-update and power phases, so freezing or resuming affects those
phases on different frames. The playback scene is retained while frozen; the
interface continues drawing. Options / View Demo still exits on live input, as
the original attract-mode owner does. There is no native tape recorder UI.

`--inspect-demo` has its own camera override on **Ctrl+Shift+arrows**; adding Alt
changes distance. This replaces the earlier Ctrl+arrows override, which prevented
the original turning commands from reaching the HERC. Ctrl+F1/F2/F3 still select
the inspection camera mode.

The new original-executable fixture checks 2,160 repositionings, 6,000 throttle
transforms and 600 damage budgets across both editions. Global routing captures
check modifier dispatch, freeze gating, the expansion's Ctrl+1 no-op and 64 pod
destinations. Live mission checks cover both editions, command routing to a
squadmate, freeze/resume phase order, retained world rendering and the distinction
between assigned and unassigned pods. See [fixture provenance](../tests/fixtures/README.md).

## Manual comparison and remaining WCS input coverage

F11, N and L are documented gameplay keys. The original Ctrl+Alt+P effects,
direct repositioning, step selection and recording freeze are absent from the
supplied player instructions. B, Z and Enter's external-view alias are other
unlisted controls. The analysis repository's `docs/CHEATS_AND_DEVELOPMENT.md`
contains the manual page references, controller-profile evidence and the full
classification of identified controls and internal launch switches.

Ctrl+1 is a controller-specific exception. The base ES.ADV assigns it to reverse
direction. The expansion manual and README document Ctrl+1 through Ctrl+0 as
WCS throttle presets. Callback `0x51c28` handles those before global dispatch;
`0x51f84` combines the retained level with current throttle in the WCS input
calculation. The native port currently supplies a modern physical throttle axis
and lacks that complete keyboard/feedback route. The earlier statement that
expansion Ctrl+1 simply does nothing described only its global handler and
ordinary play with WCS disabled. Original callback captures now establish this
remaining input work; the 306 passing tests did not cover the WCS preset path.
