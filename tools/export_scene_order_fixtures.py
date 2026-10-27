#!/usr/bin/env python3
"""Import original whole-scene painter ordering references."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1])/'worlds/native-order.json').read_text())
out = bytearray(b'ESORDER1') + struct.pack('<I', len(rows))
for row in rows:
    out += struct.pack('<3iH', *row['eye'], len(row['entries']))
    for entry in row['entries']:
        out += struct.pack('<3iiB6i', *entry['position'], entry['distance'], entry['kind'], *entry['first'], *entry['second'])
    out += struct.pack('<'+'H'*len(row['order']), *row['order'])
(root/'tests/fixtures/scene_order.bin').write_bytes(out)
print(len(rows), 'original scene trees imported')
