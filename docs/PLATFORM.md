# Platform implementation

Earthsiege uses GLFW for its window, keyboard, mouse and joystick input,
and miniaudio for PCM output. The versions and build approach match Darker and
TFX. SDL is no longer a dependency.

The game still draws every original 320 x 200 pixel in software. OpenGL 2.1
uploads that completed image to a nearest-filtered texture. It does not draw the
models, choose visibility, shade surfaces or change the original renderer.
The window uses square pixels, integer scaling where the display permits it,
and black borders after resizing. One viewport transform handles framebuffer
size, logical mouse positions and cursor warps, including HiDPI displays.

GLFW callbacks append to an ordered input queue. A mission modal can stop
consuming that queue and resume it at the original simulation call site, so a
quick Pause/Escape pair retains its order. Character input is separate from
physical key commands. Focus loss clears held controls and pauses audio.
Ctrl+Shift+M opens the in-window music selector; keyboard and mouse selection,
Cancel and missing-device errors use the same GLFW window.

The first two connected joysticks retain their slots until disconnected. Raw
GLFW axes are converted to the existing 0-65535 host range, preserving the
original device profiles, saved host calibrations, throttle, pedal fallback and
hat mappings. GLFW's synthetic hat buttons are disabled so they cannot become
extra weapon buttons. A test source exercises device connection, removal and
sample conversion without requiring physical controllers on CI.

The game's existing mixer and synthesiser backends produce signed 16-bit,
44.1 kHz stereo samples. A synchronized queue feeds those samples to miniaudio.
Underruns emit silence. Pause retains queued samples, and clearing the queue
removes audio belonging to the previous scene. Miniaudio performs any conversion
needed by the host output device.

Existing saves and preferences remain in the same locations:

- Linux: `$XDG_DATA_HOME/lost-engines/earthsiege`, or
  `~/.local/share/lost-engines/earthsiege`.
- Windows: `%APPDATA%/lost-engines/earthsiege`.
- macOS: `~/Library/Application Support/lost-engines/earthsiege`.

`--save-dir` still overrides that location. Native executable-path discovery
finds bundled assets and optional ROMs even when launched outside the package
directory. ROM discovery keeps its existing rules documented in [MUSIC.md](MUSIC.md).

GCC compiles C and C++ in the macOS release build. As in the sibling projects,
Apple Clang compiles the Cocoa and CoreAudio sources as Objective-C. The game
continues to target C++26.

## Checks

`platform_transport` covers square-pixel viewports, HiDPI cursor round trips,
letterbox input and queued PCM pause/clear/underrun behaviour. `joystick_devices`
covers host-device selection and the original axis/button/hat mappings.

Configure with `-DEARTHSIEGE_TEST_WINDOW=ON` to add `platform_window`, which needs
a desktop display. It exercises the actual GLFW callbacks, event ordering,
Unicode text, focus, mouse coordinates and close events, then reads back the
OpenGL presentation to check orientation and colour channels. It is an opt-in
local test; CI disables real-window checks. Package execution checks are also
local-only, selected explicitly with `--run` or `--window`.

The migration passed all 272 standard tests and the additional window check.
A separate X11 keyboard check exercised music selection, saved preferences,
Cancel and recovery from a missing SoundFont. The original-executable comparison
suite continues to cover the unchanged gameplay, renderer and music scheduling.
