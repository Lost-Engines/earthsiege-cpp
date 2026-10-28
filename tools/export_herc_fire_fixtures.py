#!/usr/bin/env python3
"""Export original HERC lead, retention and firing decisions."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-fire-control.json').read_text())
out = bytearray(b'ESHFI001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
def words(values): pack('H', len(values)); pack('h' * len(values), *values)
pack('H', len(data['cases']))
for row in data['cases']:
    out.extend(row['name'].encode('ascii') + b'\0'); words(row['equipment']); words(row['values']); pack('H', len(row['cases']))
    for c in row['cases']:
        pack('H', len(c['states']))
        for s in c['states']:
            pack('B', s is not None)
            if s is not None: pack('B4h', s['disabled'], s['remaining'], s.get('setting', -1), s.get('charge', -1), s.get('rounds', -1))
        pack('2h4B6iH5h56h2B', c['retained'], c['chosen'], c['skip'], c['enemy'], c['campaign'], c['difficulty'], *c['position'], *c['point'], c['heading'], c['facing'], c['speed'], *c['errors'], *c['random'])
        pack('B', len(c['events']))
        for event in c['events']:
            kind = ['choose', 'speed', 'aim', 'fire'].index(event[0])
            pack('4i', kind, *event[1:], *([0] * (4 - len(event))))
        pack('hB56h2B', c['final_retained'], c['final_skip'], *c['final_random'])
(root / 'tests/fixtures/herc_fire_control.bin').write_bytes(out)
print(sum(len(r['cases']) for r in data['cases']), 'original fire decisions exported')
