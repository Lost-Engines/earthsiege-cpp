#!/usr/bin/env python3
"""Import native scenery construction and independently decoded world settings."""
import json
import struct
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])
rows = json.loads((analysis/'missions/native-zones.json').read_text())
out = bytearray(b'ESWORLD1') + struct.pack('<I', len(rows))
for row in rows:
    metadata = bytes.fromhex(row['metadata'])
    out += row['resource'].encode() + b'\0' + struct.pack('<HH', row['terrain'], len(metadata)) + metadata + struct.pack('<H', len(row['items']))
    for item in row['items']:
        out += struct.pack('<H3i3HB', item['shape'], *item['position'], *item['angles'], item['uses_metadata'])
worlds = json.loads((analysis/'worlds/worlds.json').read_text())
out += struct.pack('<H', len(worlds))
for row in worlds:
    out += row['source'].encode() + b'\0' + struct.pack('<12h', *row['sky'])
    for band in row['distance_bands']:
        out += struct.pack('<I', len(band)) + struct.pack('<'+'i'*len(band), *band)
    out += struct.pack('<h', row['shade_levels'])
    for key in ['near_ranges', 'far_ranges', 'blend_ranges']:
        out += struct.pack('<H', len(row[key]))
        for pair in row[key]:out += struct.pack('<2h', *pair)
    out += struct.pack('<3h2i', row['unused'], *row['other'])
    for name in row['strings']:out += name.encode() + b'\0'
(root/'tests/fixtures/world.bin').write_bytes(out)
print(len(rows), 'native zones and', len(worlds), 'environment descriptors imported')
