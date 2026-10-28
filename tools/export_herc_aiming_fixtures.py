#!/usr/bin/env python3
"""Export original HERC aim and convergence traces."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-aiming.json').read_text())
out = bytearray(b'ESH A001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
actors = [a for world in data['worlds'] for a in world['actors']]
pack('H', len(actors))
for actor in actors:
    pack('8hH', actor['height'], *actor['yaw_profile'], *actor['pitch_profile'], len(actor['offsets']))
    for offset in actor['offsets']:
        pack('B', offset is not None)
        if offset is not None: pack('3h', *offset)
    pack('H', len(actor['cases']))
    for case in actor['cases']:
        pack('Bi3i', case['centre'], case['distance'], *case['point'])
        for field in ['placement', 'pivot']: pack('9hBB3i', *case[field])
        pack('h4h3h', case['ratio'], *case['old'], *case['errors'])
        assert len(case['requests']) == 2 and case['requests'][0][0] == 'yaw' and case['requests'][1][0] == 'pitch'
        pack('hih4h', case['requests'][0][1], *case['requests'][1][1:], *case['final'])
        for angles in case['convergence']:
            if angles is not None: pack('3h', *angles)
(root / 'tests/fixtures/herc_aiming.bin').write_bytes(out)
print(sum(len(a['cases']) for a in actors), 'original aim updates exported')
