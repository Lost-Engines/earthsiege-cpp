#!/usr/bin/env python3
"""Import complete original effect/scenery drawings, including light registration."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = json.loads((args.analysis / 'models/effect-render.json').read_text())
with (root / 'tests/fixtures/effect_render.bin').open('wb') as out:
    def pack(fmt, *values):
        out.write(struct.pack('<' + fmt, *values))
    out.write(b'ESFXRN01')
    pack('10hH', *data['header'], len(data['cases']))
    for row in data['cases']:
        rng = row['initial_random']
        pack('56H2BH', *[x & 65535 for x in rng[:56]], *rng[56:], len(row['requests']))
        for request in row['requests']:
            pack('5i', *request)
        pack('H', len(row['frames']))
        for frame in row['frames']:
            pack('B3i3HI', frame['updates'], *frame['position'], *frame['angles'], frame['crc32'])
print(sum(len(r['frames']) for r in data['cases']), 'complete original effect/scenery drawings imported')
