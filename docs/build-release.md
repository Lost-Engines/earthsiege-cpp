# Builds and releases

The [Build and release workflow](https://github.com/Lost-Engines/earthsiege-cpp/actions/workflows/build.yml)
builds Debug and Release configurations for Linux x86-64, Windows x86-64 and
macOS Apple Silicon. All six jobs run the CTest suite against the bundled game
resources and original-executable captures. No sibling research checkout or DOSBox
installation is needed. Tests requiring optional Roland ROMs and a SoundFont remain
opt-in; CI does not download these assets.

Each job installs the Runtime component, creates an archive, extracts it into a
temporary directory and launches it there. The checks cover automatic data discovery,
a headless mission frame and the ROM helper's help output. Linux also exercises
a real GLFW window under Xvfb/Mesa, including framebuffer readback and input
callbacks. Windows checks run without MinGW on the executable search path.
Test reports and both configurations' packages are retained as workflow artifacts
for seven days.

## Downloads

The [latest release](https://github.com/Lost-Engines/earthsiege-cpp/releases/latest)
contains:

- `earthsiege-cpp-linux-x86_64.tar.gz` (built on Ubuntu 24.04).
- `earthsiege-cpp-windows-x86_64.zip`.
- `earthsiege-cpp-macos-arm64.tar.gz` (macOS 15 or newer).
- `SHA256SUMS` covering the three archives.

Extract the whole archive and run `earthsiege` or `earthsiege.exe`. Keep the `assets`
directory beside the executable. The original freeware game data is included.
GLFW, miniaudio, the synthesiser backends and compiler runtimes are linked statically;
operating-system libraries and Linux display/audio drivers remain system dependencies.
These archives are not signed or notarised.

OPL music works immediately. The optional Roland firmware helper is in `scripts`;
see [music setup](MUSIC.md). ROMs, SoundFonts, saves and local research are excluded
from the downloads. Third-party notices are included under `third_party`.

## Publication

Code, dependency or bundled-data changes pushed to `master` publish a release only
after all six build, test and package jobs succeed. Changes limited to documentation,
tests, tools or the workflow still run CI, but do not automatically publish.

To publish a tested packaging-only change, use **Actions > Build and release > Run
workflow**, select `master` and enable **Publish release packages after all builds
and tests pass**. Leaving that option off builds artifacts without publishing.
Pull requests never publish releases.

Release tags are `release-<full commit SHA>`. Packages are uploaded to a draft first;
the completed release then becomes the latest download. Older automated releases
are removed after publication, while their Git tags, manual releases and drafts
are retained. A slower build of an older commit cannot replace a newer release.

## Reproduce a portable build

Use CMake 3.30 or newer, Ninja and a compiler supporting C++26. CI uses GCC 16 on
Linux and macOS, and the UCRT64 GCC toolchain on Windows. macOS also uses Apple's
Clang for the Cocoa/CoreAudio sources; C and C++ use GCC. The workflow lists the native development
packages needed to build GLFW on Linux.

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_glfw3=ON -DEARTHSIEGE_STATIC_RUNTIME=ON
cmake --build build-release --parallel 3
ctest --test-dir build-release --output-on-failure --parallel 3 --timeout 600
cmake --install build-release --prefix "$PWD/build-release/package/earthsiege-cpp" --component Runtime
cmake -DPACKAGE_ROOT="$PWD/build-release/package" -DOUTPUT_DIR="$PWD/build-release/dist" \
  -DPLATFORM=linux-x86_64 -P cmake/package.cmake
python3 tools/check_package.py build-release/dist/earthsiege-cpp-linux-x86_64.tar.gz
```

Use `windows-x86_64` or `macos-arm64` when packaging for the corresponding host.
The package checker requires Python 3.12 or newer. Ordinary development builds
use a system GLFW 3.4 when available; otherwise CMake builds the pinned version.
Dependencies are pinned and hash-checked; [THIRD_PARTY.md](../THIRD_PARTY.md)
describes offline source overrides.
