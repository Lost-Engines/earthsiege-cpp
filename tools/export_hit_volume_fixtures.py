#!/usr/bin/env python3
"""Import original sphere-list selection and directional shield absorption."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
report = json.loads((Path(sys.argv[1]) / 'models/native-collision.json').read_text())
out = bytearray(b'ESHIT001') + struct.pack('<I', len(report['cases']))
for row in report['cases']:
    kind = row['kind']
    if kind == 'sphere':
        out += struct.pack('<Bi2h3iBBi', 0, row['reach'], row['projectile_radius'], row['radius'], *row['point'], row['update'], *row['result'])
    elif kind == 'sphere_list':
        out += b'\1' + row['model'].encode() + b'\0'
        out += struct.pack('<2hH3iih2i', row['part'], row['state'], row['angle'], *row['translation'], row['reach'], row['projectile_radius'], *row['result'])
    else:
        result, damage, halves = row['result']
        out += struct.pack('<BH3ii3hih2h', 2, row['angle'], *row['translation'], row['reach'], row['damage'], *row['shields'], result, damage, *halves)
(root / 'tests/fixtures/hit_volumes.bin').write_bytes(out)
print(len(report['cases']), 'original projectile collision cases')
