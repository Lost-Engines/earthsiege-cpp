#!/usr/bin/env python3
"""Import original HERC control traces and fixed-point direction angles."""
import json
import struct
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])
def state(row):
    return struct.pack('<3i3H4h', *row['position'], *row['angles'], row['throttle'], row['speed'], row['turn'], row['animation_rate'])
def channel(row):
    return struct.pack('<15hH', *row['channel'], row['flags'])

rows = [json.loads(p.read_text()) for p in sorted((analysis/'models/locomotion').glob('*.json')) if p.stem != 'catalogue']
out = bytearray(b'ESLOCO01') + struct.pack('<I', len(rows))
count = 0
for row in rows:
    out += row['model'].encode() + b'\0' + state(row['initial']) + channel(row['initial']) + struct.pack('<H', len(row['traces']))
    for trace in row['traces']:
        out += trace['control'].encode() + b'\0' + struct.pack('<hH', trace['ratio'], len(trace['states']))
        for inputs, expected in zip(trace['inputs'], trace['states'], strict=True):
            out += struct.pack('<2h', *inputs) + state(expected) + channel(expected)
            count += 1
(root/'tests/fixtures/locomotion.bin').write_bytes(out)
rows = json.loads((analysis/'models/native-direction-angles.json').read_text())['cases']
out = bytearray(b'ESANGLE1') + struct.pack('<I', len(rows))
for x, y, angle in rows:
    out += struct.pack('<2iH', x, y, angle)
(root/'tests/fixtures/direction_angles.bin').write_bytes(out)
print(count, 'original locomotion steps and', len(rows), 'original direction angles imported')
