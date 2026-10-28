#!/usr/bin/env python3
"""Import original beam geometry, random state and complete scene frames."""
import json
from pathlib import Path
import struct
import sys

root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'models/beam-effects.json').read_text())
with (root / 'tests/fixtures/beam_effects.bin').open('wb') as out:
    def pack(fmt, *v): out.write(struct.pack('<' + fmt, *v))
    def rng(v): pack('56H2B', *[x & 65535 for x in v[:56]], *v[56:])
    out.write(b'ESBEAM01')
    pack('10hH', *data['header'], len(data['cases']))
    for row in data['cases']:
        pack('h3i3iB', row['subtype'], *row['start'], *row['end'], row['colour'])
        rng(row['initial_random']); rng(row['random'])
        pack('H', len(row['points']))
        for point in row['points']: pack('3i', *point)
        pack('H', len(row['faces']))
        for face in row['faces']: pack('4H', *face)
        pack('B3i3HI', row['expired'], *row['position'], *row['angles'], row['crc32'])
print(len(data['cases']), 'original beam geometry and drawing cases imported')
