#!/usr/bin/env python3
"""Import original light calculations, transforms and capacity results."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = json.loads((args.analysis / 'models/native-lighting.json').read_text())
with (root / 'tests/fixtures/model_lighting.bin').open('wb') as out:
    def light(row):
        out.write(struct.pack('<Bi3i3h2i', row['kind'], row['intensity'], *row['position'], *row['direction'], row['attenuation'], row['scale']))
    out.write(b'ESLIGHT1')
    light(data['baseline'])
    out.write(struct.pack('<H', len(data['sets'])))
    for group in data['sets']:
        out.write(struct.pack('<H', len(group)))
        for row in group:
            light(row)
    out.write(struct.pack('<I', len(data['cases'])))
    for row in data['cases']:
        out.write(struct.pack('<H3i3h3B', row['lights'], *row['point'], *row['normal'], row['model'], row['terrain'], row['unoriented']))
    out.write(struct.pack('<I', len(data['transforms'])))
    for row in data['transforms']:
        out.write(struct.pack('<9hB3i', *row['basis'], row['type'], *row['translation']))
        light(dict(data['baseline'], position=row['position'], direction=row['direction']))
        light(row['result'])
    out.write(struct.pack('<H', len(data['list_operations'])))
    for row in data['list_operations']:
        out.write(struct.pack('<HBB', row['index'], row['accepted'], row['count']))
print(len(data['cases']) * 3, 'original intensities and', len(data['transforms']), 'light transforms imported')
