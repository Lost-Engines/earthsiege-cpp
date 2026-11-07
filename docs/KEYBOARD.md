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

## Original shortcuts still outside the implementation

These executable handlers are known, but the native port does not yet reproduce
them. They should not be confused with missing letter-to-scan mappings.

- Ctrl+1 changes the forward/reverse control regime and throttle limits. The
  normal unrestricted regime is implemented; this alternate mode is not.
- Alt+arrow keys reposition the HERC directly; Ctrl+Left/Right turns it by a
  fixed step. Alt+Ctrl+1 through 9 selects that step. The native inspection
  camera currently uses Ctrl+arrows instead.
- Alt+Ctrl+P invokes a development damage-balancing routine. Its original branch
  is identified, but its full execution has not been validated for a native port.
- Alt+S pauses the original recording/playback facility only when that facility
  is active. The native shell's View Demo instead treats live input as a request
  to leave playback; it has no recording interface.

The analysis repository's `docs/CHEATS_AND_DEVELOPMENT.md` gives the original
addresses and the 216 captured repositioning/turn commands. Those captures
document the DOS executable; they do not imply these controls are implemented
in C++.
