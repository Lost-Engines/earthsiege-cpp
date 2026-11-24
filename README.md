# Earthsiege C++

*Metaltech: Earthsiege* is a 1994 mech simulator by Dynamix. You pilot an armoured walker called a HERC, lead your squad against the Cybrids and salvage parts to get your machines ready for the next mission. This version recreates the DOS game and its expansion for modern computers.

## Getting started

1. Download the package for Windows, Linux or Mac (Apple Silicon) from the [latest release](https://github.com/Lost-Engines/earthsiege-cpp/releases/latest).
2. Extract the whole archive. It includes the game and expansion data.
3. On Windows, double-click `earthsiege.exe`. On Linux or macOS, run `./earthsiege` from the extracted folder.
4. Choose **Earthsiege** or **Earthsiege Expansion** when the game opens.

### Optional Roland music

The default Sound Blaster music works as soon as you start the game. If you'd like to hear the Roland arrangements, the fetch-assets script downloads and verifies the ROMs for the SC-55, SCC-1A and CM-32L.

- On Windows, open the `scripts` folder and double-click `fetch-assets.bat`.
- On Linux or macOS, from the extracted game folder, run `./scripts/fetch-assets.sh`.

The game finds the downloaded ROMs automatically. Press **Ctrl+Shift+M** to choose a music device.

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
