# Earthsiege C++

*Metaltech: Earthsiege* is a 1994 mech simulator by Dynamix. You pilot an armoured walker called a HERC, lead your squad against the Cybrids and salvage parts to get your machines ready for the next mission. This version recreates the DOS game and its expansion for modern computers.

## Playing the game

Earthsiege and its expansion are bundled in the [Windows, Linux and Apple Silicon Mac downloads](https://github.com/Lost-Engines/earthsiege-cpp/releases/latest).

## Building from source

You need Git, CMake 3.30 or newer, Ninja and a C++26 compiler such as GCC 16. The checkout includes the game data. CMake downloads GLFW and the audio libraries the first time you configure it.

<details>
<summary>Platform prerequisites</summary>

- Linux needs the OpenGL, X11 and Wayland development files. If you have several GCC versions, `export CC=gcc-16 CXX=g++-16` selects GCC 16. On Debian or Ubuntu, install the display dependencies with

  ```sh
  sudo apt install pkg-config libgl-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev wayland-protocols
  ```

- On Windows, use the MSYS2 UCRT64 shell. Install the tools with `pacman -S --needed git mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,pkgconf}`.
- On macOS, install the Xcode Command Line Tools and run `brew install cmake ninja gcc@16`. Before configuring, run `export PATH="$(brew --prefix gcc@16)/bin:$PATH" CC=gcc-16 CXX=g++-16 OBJC="$(xcrun --find clang)"`.

</details>

```sh
git clone https://github.com/Lost-Engines/earthsiege-cpp.git
cd earthsiege-cpp
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --parallel
./build/earthsiege
```

### Optional Roland music

Sound Blaster music works immediately. To use Roland music, run the fetch-assets script once from the repository or extracted release. It downloads and verifies the SC-55, SCC-1A and CM-32L ROMs.

| Platform | Command |
| --- | --- |
| Linux or macOS | `./scripts/fetch-assets.sh` |
| Windows (PowerShell or Command Prompt) | `.\scripts\fetch-assets.bat` |

The game finds the default ROM location automatically. Press **Ctrl+Shift+M** to choose a music device. To keep the ROMs elsewhere, give the script a destination folder and pass that folder to the game with `--rom-dir PATH`.

## Command-line options

| Option | What it does |
| --- | --- |
| `--original` or `--expansion` | Skip the edition menu and start the chosen game. |
| `--skip-intro` | Skip the opening and pilot-death cinematics. |
| `--demo N` | Watch demo 1 to 4. Add `--expansion` for the expansion's recordings. |
| `--inspect-demo N` | Take the controls in the starting mission from demo 1 to 4. |
| `--scale N` | Set the window size to a multiple of 320×200. The default is the largest scale that fits your screen. |
| `--audio-setup` | Open the music device selector at startup. |
| `--music NAME` | Choose `opl2`, `opl3` (default), `sc55`, `scc1a`, `mt32`, `mt32-mpu` or `soundfont`. |
| `--soundfont FILE` | Use an SF2 file with the `soundfont` music option. |
| `--rom-dir PATH` | Look for Roland ROMs in this folder. |
| `--data-dir PATH` | Use game data from another installation. |
| `--save-dir PATH` | Choose where to store saved games. |
| `--screenshot FILE` | Save the final frame as a 320×200 PPM image. |
| `--frames N` | Quit after rendering N frames. |
| `--headless` | Render without opening a window. Defaults to one frame. |
| `--list-resources` | Print the game files, their sizes and checksums. |
| `--help` | Show the available options. |
