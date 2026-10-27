#!/usr/bin/env python3
"""Import original palette remaps and independently verified ramp results."""
import argparse
import json
from pathlib import Path
import struct
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('research', type=Path)
args = parser.parse_args()
sys.path.insert(0, str(args.research / 'tools'))
from three_space import palette
root = Path(__file__).resolve().parents[1]
data = json.loads((args.research / 'analysis/models/material-shading.json').read_text())
with (root / 'tests/fixtures/palette_shading.bin').open('wb') as out:
    out.write(b'ESSHADE1')
    out.write(struct.pack('<H', len(data['textures'])))
    for key, maps in data['textures'].items():
        out.write(key.encode() + b'\0')
        for level in maps:
            out.write(bytes(level))
        decoded = palette((args.research / 'extracted/cd_1994_10/resources' / key).read_bytes())
        # check_materials.py verified these exact lookups with original 0x68be0.
        ramps = {len(r): i for i, r in enumerate(decoded['ramps']) if r}
        out.write(struct.pack('<H', len(ramps)))
        for size, index in ramps.items():
            out.write(struct.pack('<H', index))
            out.write(bytes(decoded['ramps'][index][i * size >> 8] for i in range(256)))
print(data['ramp_cases'], 'verified ramp results and', len(data['textures']) * 8, 'original texture maps imported')
