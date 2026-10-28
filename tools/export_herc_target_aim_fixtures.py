#!/usr/bin/env python3
"""Export original torso-cone and aim-point decisions."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-target-aim.json').read_text())
out = bytearray(b'ESHTA001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
rows = [r for w in data['worlds'] for r in w['cases']]
pack('I', len(rows))
for r in rows:
    pack('B3iHh3ih3i', r['kind'], *r['origin'], r['heading'], r['limit'], *r['position'], r['component'], *r['centre_position'])
    if r['component'] >= 0: pack('3i', *r['part_position'])
    assert len(r['events']) == 1
    event = r['events'][0]
    if event[0] == 'centre':
        assert event[1] == 0
        pack('B', 0)
    else:
        assert event[1] == r['facing']
        pack('B3i', 1, *event[2:])
(root / 'tests/fixtures/herc_target_aim.bin').write_bytes(out)
print(len(rows), 'aim target decisions exported')
