#!/usr/bin/env python3
"""Import complete native textured-polygon VGA framebuffer checksums."""
import argparse
import json
from pathlib import Path
import struct
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = json.loads((args.analysis / 'models/native-texture-raster.json').read_text())
with (root / 'tests/fixtures/texture_raster.bin').open('wb') as out:
    out.write(b'ESTEXTU2')
    out.write(struct.pack('<HHBI', data['width'], data['height'], data['background'], len(data['cases'])))
    out.write(bytes(data['texture']))
    out.write(bytes(data['remap']))
    for row in data['cases']:
        out.write(struct.pack('<H', len(row['points'])))
        for p,uv in zip(row['points'],row['uv']):out.write(struct.pack('<4i', *p, *uv))
        out.write(struct.pack('<4iBBI', *row['bounds'], row['mapped'], row['scan_rows'], row['crc32']))
print(len(data['cases']), 'complete original texture rasters imported')
