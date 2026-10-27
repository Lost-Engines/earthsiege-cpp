#!/usr/bin/env python3
"""Import original distance-band, culling and per-object palette references."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1])/'worlds/native-distance-shading.json').read_text())
out = bytearray(b'ESDIST01') + struct.pack('<H', len(rows))
for row in rows:
    out += struct.pack('<HI', row['world'], len(row['cases']))
    for case in row['cases']:
        out += struct.pack('<hihBHBB', case['kind'], case['distance'], case['radius'], case['split'], case['band'], case['culled'], len(case['ramps']))
        for slot, values in case['ramps']:
            out += struct.pack('<BH', slot, len(values)) + struct.pack('<'+'H'*len(values), *values)
(root/'tests/fixtures/distance_shading.bin').write_bytes(out)
print(sum(len(r['cases']) for r in rows), 'original distance/shading cases imported')
