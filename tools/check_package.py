#!/usr/bin/env python3
"""Check an extracted release, away from the checkout and toolchain libraries."""

import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import zipfile


def check(archive):
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
            "third_party/licenses/SDL2-zlib.txt", "third_party/nuked-opl3/LICENSE",
            "third_party/licenses/Munt-LGPL-2.1.txt",
            "third_party/licenses/Munt-GPL-2.0.txt",
            "third_party/licenses/Nuked-SC55-GPL-2.0.txt",
            "third_party/licenses/TinySoundFont-MIT.txt",
        ]
        required += [f"assets/VOL/{name}.VOL" for name in
                     ("DETECT", "SHELL0", "SIMSOUND", "SIMVOICE", "SIMVOL0", "SPEECH", "ZONES")]
        required += [f"assets/TAPES/DEMO{n}.TAP" for n in range(1, 5)]
        for name in required:
            if not (game / name).is_file():
                raise RuntimeError(f"Missing package file: {name}")
        for path in game.rglob("*"):
            if path.suffix.lower() in (".rom", ".sf2") or path.name == ".git":
                raise RuntimeError(f"Unexpected local asset in package: {path}")
        environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
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
        # No --data-dir: the unpacked game must discover its own bundled resources.
        run("--headless", "--inspect-demo", "1", "--frames", "1", "--music", "opl3",
            "--save-dir", str(root / "saves"), "--screenshot", str(root / "frame.ppm"))
        if not (root / "frame.ppm").read_bytes().startswith(b"P6\n320 200\n255\n"):
            raise RuntimeError("Packaged renderer did not produce a 320 x 200 frame")
        # Exercise SDL startup and presentation without needing a physical display.
        run("--skip-intro", "--frames", "2", "--music", "opl3",
            "--save-dir", str(root / "saves"))
        if os.name != "nt":
            subprocess.run([str(game / "scripts/fetch-assets.sh"), "--help"],
                           cwd=root, check=True, timeout=10)
        print(f"Package passed: {archive.name}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: check_package.py PACKAGE.tar.gz|PACKAGE.zip")
    check(Path(sys.argv[1]).resolve())
