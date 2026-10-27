#!/usr/bin/env python3
"""Import original scenery heights, stepped intersections and scene ray queries."""
import json
import struct
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])/'models'
local = json.loads((analysis/'scenery-collision.json').read_text())['results']
scenes = {(r['resource'], r['root']): r for r in json.loads((analysis/'scenery-rays.json').read_text())['checks']}
out = bytearray(b'ESCOLL01') + struct.pack('<I', len(local))
for row in local:
    out += row['resource'].encode() + b'\0' + struct.pack('<H', row['root'])
    out += struct.pack('<I', len(row['queries']))
    for point, radius, height in row['queries']:
        out += struct.pack('<2ihi', *point, radius, height)
    out += struct.pack('<I', len(row['segments']))
    for a, b, radius, hit in row['segments']:
        out += struct.pack('<6ihB', *a, *b, radius, hit is not None)
        if hit is not None:
            out += struct.pack('<3i', *hit)
    cases = scenes[(row['resource'], row['root'])]['cases']
    out += struct.pack('<I', len(cases))
    for case in cases:
        out += struct.pack('<H3iihH', case['heading'], *case['origin'], case['reach'], case['radius'], len(case['objects']))
        for position, angle in case['objects']:
            out += struct.pack('<3iH', *position, angle)
        out += struct.pack('<hi', case['selected'] if case['selected'] is not None else -1, case['hit_distance'] or 0)
(root/'tests/fixtures/scenery_collision.bin').write_bytes(out)
print(len(local), 'original scenery collision fields imported')
