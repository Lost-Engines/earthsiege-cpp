#!/usr/bin/env python3
"""Import original polygon edge tables and viewport clipping results."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
edges = json.loads((args.analysis / 'models/native-polygon-edges.json').read_text())['cases']
clips = json.loads((args.analysis / 'models/native-viewport-clip.json').read_text())['cases']
solid = json.loads((args.analysis / 'models/native-solid-raster.json').read_text())['cases']
with (root / 'tests/fixtures/polygon_raster.bin').open('wb') as out:
    def points(values):
        out.write(struct.pack('<H', len(values)))
        for p in values: out.write(struct.pack('<2i', *p))
    out.write(b'ESRASTE2')
    out.write(struct.pack('<III', len(edges), len(clips), len(solid)))
    for row in edges:
        points(row['points'])
        out.write(struct.pack('<hH', row['y'], len(row['spans'])))
        for span in row['spans']:out.write(struct.pack('<2h', *span))
    for row in clips:
        points(row['points'])
        out.write(struct.pack('<4i', *row['bounds']))
        points(row['result'])
    for row in solid:
        points(row['points'])
        out.write(struct.pack('<4iBI', *row['bounds'], row['colour'], row['crc32']))
print(len(edges), 'original edge tables and', len(clips), 'viewport clips imported')
