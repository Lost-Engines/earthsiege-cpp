# Third-party code

Earthsiege C++ is GPLv3-or-later. The original game assets and synthesiser ROMs
have separate ownership and are not covered by the code licence.

| Component | Source and revision | Licence |
|---|---|---|
| Nuked OPL3 | Vendored in `third_party/nuked-opl3`, by Alexey Khokholov (Nuke.YKT). | LGPL-2.1-or-later; notice and licence retained with the source. |
| Munt / libmt32emu | [munt/munt](https://github.com/munt/munt), `5ee078a761d35cdd3d27000b551c12d11df6a60b`. Dean Beeler, Jerome Fisher, Sergey V. Mikayev and contributors. | LGPL-2.1-or-later. |
| Nuked-SC55 | [jcmoyer/Nuked-SC55](https://github.com/jcmoyer/Nuked-SC55), `f3464753f64a7da5f5fd3a96fd72197628369589`. Alexey Khokholov, J. C. Moyer and contributors. | GPL-2.0-or-later. |
| TinySoundFont | [schellingb/TinySoundFont](https://github.com/schellingb/TinySoundFont), `853a0a171759f1ddba0de1442133a75912bbeffa`. Bernhard Schelling, based on SFZero by Steve Folta. | MIT. |
| GLFW | [glfw/glfw](https://github.com/glfw/glfw), `a74efa0d5628b74adc0426af4c5710e287fa7c2c` (3.4), or system GLFW 3.4. | zlib. |
| miniaudio | [mackron/miniaudio](https://github.com/mackron/miniaudio), `f40cf03f80cdb7e741d43e53b7e706e8c1394bcf`. David Reid and contributors. | Public domain or MIT-0. |

CMake downloads the three pinned synthesis libraries with SHA-256 verification.
Their source archives retain upstream copyright and licence notices. Copies of
the synthesis licences are in [third_party/licenses](third_party/licenses).
The platform layer uses the same pinned GLFW and miniaudio sources as Darker and
TFX. Their downloads are hash-checked and their notices are included in packages.
For offline builds, populate CMake's FetchContent cache or set the corresponding
`FETCHCONTENT_SOURCE_DIR_MUNT`, `FETCHCONTENT_SOURCE_DIR_NUKED_SC55` and
`FETCHCONTENT_SOURCE_DIR_TINYSOUNDFONT` source directories (plus
`FETCHCONTENT_SOURCE_DIR_GLFW` and `FETCHCONTENT_SOURCE_DIR_MINIAUDIO`).

The Munt and Sound Canvas adapters, backend build definitions and ROM-fetch
helpers were adapted from the sibling Lost Engines TFX and Darker projects,
under their GPLv3-or-later licence. Earthsiege's HMI scheduling, driver routing,
gain and sampled-percussion integration follow its own original executable;
those parts are not imported game logic from TFX or Darker.

The ROM helper downloads firmware separately and verifies the hashes in
`scripts/roms.txt`. ROMs and SoundFont banks are not included in the source tree.
