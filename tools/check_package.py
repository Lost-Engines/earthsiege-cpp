#!/usr/bin/env python3
"""Validate package contents. Executing the packaged game is a local opt-in."""

import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import zipfile


def check(archive, run_game=False, window=False):
    with tempfile.TemporaryDirectory(prefix="earthsiege-package-") as temporary:
        root = Path(temporary)
        if archive.suffix == ".zip":
            with zipfile.ZipFile(archive) as package:
                package.extractall(root)
        else:
            with tarfile.open(archive) as package:
                package.extractall(root, filter="data")
        game = root / "earthsiege-cpp"
        required = [
            "README.md", "THIRD_PARTY.md", "docs/MUSIC.md", "docs/build-release.md",
            "assets/DATA/KEYJOY.CFG", "scripts/roms.txt",
            "third_party/licenses/GLFW-zlib.txt", "third_party/licenses/miniaudio.txt", "third_party/nuked-opl3/LICENSE",
            "third_party/licenses/Munt-LGPL-2.1.txt",
            "third_party/licenses/Munt-GPL-2.0.txt",
            "third_party/licenses/Nuked-SC55-GPL-2.0.txt",
            "third_party/licenses/TinySoundFont-MIT.txt",
        ]
        required += [f"assets/VOL/{name}.VOL" for name in
                     ("DETECT", "SHELL0", "SIMSOUND", "SIMVOICE", "SIMVOL0", "SPEECH", "ZONES")]
        required += [f"assets/TAPES/DEMO{n}.TAP" for n in range(1, 5)]
        required += [f"assets/expansion/VOL/{name}.VOL" for name in
                     ("DETECT", "SHELL0", "SIMSOUND", "SIMVOICE", "SIMVOL0", "SPEECH", "ZONES",
                      "SIMPATCH", "PSHELL0", "PSHELL1", "PSHELL2", "PZONES", "PZONES1",
                      "SIMLANG", "PSPEECH")]
        required += [f"assets/expansion/TAPES/DEMO{n}.TAP" for n in range(1, 5)]
        required += [f"assets/expansion/FLIC/{name}" for name in
                     ("MI_INT1.DFL", "MECHVIC3.DFL", "XPINTROB.RAW", "VICTORY.RAW")]
        required += ["assets/expansion/DATA/KEYJOY.CFG", "assets/expansion/SHA256SUMS",
                     "assets/expansion/README.md", "docs/EXPANSION.md",
                     "earthsiege.exe" if os.name == "nt" else "earthsiege",
                     "scripts/fetch-assets.bat" if os.name == "nt" else "scripts/fetch-assets.sh"]
        for name in required:
            if not (game / name).is_file():
                raise RuntimeError(f"Missing package file: {name}")
        for path in game.rglob("*"):
            if path.suffix.lower() in (".rom", ".sf2") or path.name == ".git":
                raise RuntimeError(f"Unexpected local asset in package: {path}")
        print(f"Package contents passed: {archive.name}", flush=True)
        if not (run_game or window):
            return

        environment = dict(os.environ)
        environment.pop("LD_LIBRARY_PATH", None)
        environment.pop("DYLD_LIBRARY_PATH", None)
        if os.name == "nt":
            windows = Path(os.environ["WINDIR"])
            environment["PATH"] = os.pathsep.join(map(str, (windows / "System32", windows)))
        executable = game / ("earthsiege.exe" if os.name == "nt" else "earthsiege")

        def run(*arguments):
            subprocess.run([str(executable), *arguments], cwd=root, env=environment,
                           check=True, timeout=90)

        run("--help")
        # Local opt-in only. No --data-dir: each edition must find its bundled data.
        for edition in ("original", "expansion"):
            saves = root / ("saves-" + edition)
            frame = root / (edition + ".ppm")
            run("--" + edition, "--headless", "--inspect-demo", "1", "--frames", "1",
                "--music", "opl3", "--save-dir", str(saves), "--screenshot", str(frame))
            if not frame.read_bytes().startswith(b"P6\n320 200\n255\n"):
                raise RuntimeError(f"Packaged {edition} renderer did not produce a 320 x 200 frame")
            if window:
                # Select explicitly: the normal launcher waits for a person's choice.
                run("--" + edition, "--skip-intro", "--frames", "2", "--music", "opl3",
                    "--save-dir", str(saves))
        if os.name != "nt":
            subprocess.run([str(game / "scripts/fetch-assets.sh"), "--help"],
                           cwd=root, check=True, timeout=10)
        print(f"Local package execution passed: {archive.name}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--run", action="store_true", help="Local opt-in: run headless checks for both editions")
    mode.add_argument("--window", action="store_true", help="Local opt-in: run both editions headless and windowed (requires a display)")
    parser.add_argument("archive", type=Path)
    arguments = parser.parse_args()
    check(arguments.archive.resolve(), run_game=arguments.run, window=arguments.window)
