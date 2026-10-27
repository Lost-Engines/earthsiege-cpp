#!/usr/bin/env python3
"""Import native model/terrain projections and clipped polygon attributes."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = json.loads((args.analysis / 'models/native-projection.json').read_text())
with (root / 'tests/fixtures/projection.bin').open('wb') as out:
    out.write(b'ESPROJE1')
    out.write(struct.pack('<I', len(data['cases'])))
    for row in data['cases']:
        out.write(struct.pack('<BBii2iH', row['mode'] == 'terrain', row['shift'], row['near'], row['divisor'], *row['centre'], len(row['vertices'])))
        for v in row['vertices']:
            out.write(struct.pack('<3i2iH', *v['point'], *v['uv'], v['shade']))
        out.write(struct.pack('<H', len(row['positions'])))
        for xy, uv, shade in zip(row['positions'], row['uv'], row['shade']):
            out.write(struct.pack('<2i2iH', *xy, *uv, shade))
print(len(data['cases']), 'original projected polygons imported')
