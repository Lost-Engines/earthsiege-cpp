# Music hardware

Press **Ctrl+Shift+M** in the shell or a mission to choose music hardware. The
window title includes the shortcut. A successful change restarts the current
piece and saves the choice in `music.cfg` beside the game saves. Cancelling keeps
the current choice. Effects, speech and the game's sampled percussion keep using
the original digital mixer.

OPL3 is the default and needs no extra assets. For Roland devices, run
`scripts/fetch-assets.sh` on Linux or `scripts/fetch-assets.bat` on Windows once.
The helper downloads and verifies the CM-32L, SC-55 and SCC-1A firmware sets.
It leaves the bundled game data alone. Repeating it verifies existing files and
retries any missing or damaged downloads.

| Choice | Playback |
|---|---|
| OPL2 | AdLib / Sound Blaster, original HMI FM instruments, Nuked OPL3 core in OPL2 mode. |
| OPL3 | Sound Blaster Pro / 16, original HMI FM instruments and stereo routing. |
| SC-55 | Original MPU-401/General MIDI arrangement through Nuked-SC55, SC-55 v1.21 firmware. |
| SCC-1A | The same arrangement through SCC-1A v1.30 firmware. Its instrument sounds can differ from the SC-55. |
| MT-32 / CM-32L, original selection | Original HMI MT-32 routing through Munt. In this release the melodic tracks do not target that driver, so only any sampled percussion plays. |
| MT-32 / CM-32L, MPU-401 experiment | Sends the General MIDI parts to Munt. MT-32 instrument numbers differ from GM; this is an optional listening experiment. |
| SoundFont | Original General MIDI parts through TinySoundFont and a local SF2 bank. |

## The MT-32 finding

The manual and installer advertise MT-32/LAPC-I support, and the archive contains
an MT-32 driver. However, none of the twenty supplied HMP resources assigns a
melodic part to its device ID, `0xa004`. The original song scheduler skips those
parts with the MT-32 driver selected. The separately installed digital driver,
`0xa005`, can still play the game's percussion samples.

We checked the original driver installation, scheduler, gain processing and
MPU output. The MT-32 driver sends MIDI bytes unchanged; the sound owner does not
translate GM instruments or upload an MT-32 patch bank. Both the strict selection
and the clearly named MPU-401 experiment are available so this distinction is
visible. The conclusion applies to the supplied 1994 version 1.0 data.

General MIDI coverage also varies by song. For example, DESERT has no GM melody
route. SC-55, SCC-1A and SoundFont playback preserve that omission. They do not
substitute the FM arrangement. Game percussion continues through `DRUM32.DIG`,
including when the chosen synthesiser receives no notes.

## Asset locations

On Linux the fetch helper uses `$XDG_DATA_HOME/earthsiege`, or
`~/.local/share/earthsiege` when XDG_DATA_HOME is unset or relative. This follows
the Darker and TFX asset layout. It is separate from the save directory,
`~/.local/share/lost-engines/earthsiege` by default.

The runtime searches the selected game-data directory, the executable directory
and the default asset directory, including a `roms` child of each. ROM identity
is checked by content. Munt prefers a matching CM-32L set when several sets are
present. The supplied helper installs a matching CM-32L set; an existing matching
MT-32 set also works.

On Windows the helper defaults to the directory containing the executable,
looking in `build/Release`, `build/Debug`, `build` and the project root. Before
the executable exists, it uses the bundled `assets` directory. An explicit destination
argument to either helper overrides the default. Use `--rom-dir PATH` for a
custom location outside the automatic search. An explicit path is authoritative;
an invalid one produces an error instead of silently selecting different ROMs.

For SoundFont playback, place `default.sf2` in any searched directory, or use
`--soundfont PATH`. The runtime also recognizes `default-GM.sf2`, `FluidR3_GM.sf2`
and `TimGM6mb.sf2` there, plus the system defaults
`/usr/share/sounds/sf2/default-GM.sf2` and `/usr/share/soundfonts/default.sf2`.
SoundFonts and Roland firmware are external assets, separate from the code licence.

## Command-line equivalents

```sh
./build/earthsiege --audio-setup
./build/earthsiege --music sc55
./build/earthsiege --music mt32-mpu --rom-dir /path/to/roms
./build/earthsiege --music soundfont --soundfont /path/to/bank.sf2
```

`--music` accepts `opl2`, `opl3`, `sc55`, `scc1a`, `mt32`, `mt32-mpu` and
`soundfont`. Command-line overrides apply to that run. Choosing a device in the
selector, including `--audio-setup`, saves the resulting configuration. A missing
asset during a live change leaves the previous music device available.

## Verification and timing

All supplied songs retain the original 120 Hz scheduler, device routing, loops,
volume changes and stop cleanup. A capture through the original MPU-401 and
MT-32 modules covers all twenty resources with both drivers: 30,764 output bytes
and 3,092 digital-percussion events. Native output matches every captured byte
and event. The MT-32 master-volume update uses the original `0x02fe` channel mask.

The synthesis tests exercise real CM-32L, SC-55 and SCC-1A ROMs, SoundFont notes,
the menu song, sampled percussion, pause/resume and live device changes. The
private-window check verifies selection, persistence and cancellation. See
[validation](VALIDATION.md) for results and the optional hardware-test setup.

The host stream remains 44.1 kHz. Original 11.025 kHz PCM samples are held between
DAC updates. Munt uses its accurate analogue-output mode; Sound Canvas emulation
preserves the chip clock and resamples its oversampled output. MIDI messages are
submitted on the game scheduler ticks. These checks do not measure a physical
ISA card's bus delay, analogue output balance or the phase of a particular DOS
interrupt stream.

[Third-party code and licences](../THIRD_PARTY.md) lists the synthesis libraries,
pinned versions and the adapters reused from Darker and TFX.
