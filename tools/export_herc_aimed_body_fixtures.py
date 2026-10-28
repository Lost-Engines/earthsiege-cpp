#!/usr/bin/env python3
"""Export composed original HERC aim, movement and muzzle traces."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'models/native-herc-aimed-body.json').read_text())
out = bytearray(b'ESHAB001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
def words(values): pack('H', len(values)); pack('h' * len(values), *values)
def state(s):
    pack('3i3H8hH', *s['position'], *s['angles'], *s['movement'], *s['aim'], len(s['poses']))
    for p in s['poses']: pack('3H3h', *p)
    for m in s['matrices']: pack('9hBB3i', *m)
pack('H', len(data['cases']))
for row in data['cases']:
    out.extend(row['name'].encode('ascii') + b'\0'); words(row['equipment']); words(row['values']); state(row['initial']); pack('H', len(row['cases']))
    for c in row['cases']:
        pack('3hBi3i3hh', c['ratio'], c['speed'], c['turn'], c['centre'], c['distance'], *c['point'], *c['errors'], c['slot'])
        pack('H', len(c['convergence']))
        for angles in c['convergence']:
            pack('B', angles is not None)
            if angles is not None: pack('3h', *angles)
        pack('H', len(c['shots']))
        for s in c['shots']:
            pack('Bhih3i3H9hBB3i', ['bullet', 'rocket', 'scaled', 'beam'].index(s['kind']), s['charge'], s['range'], s['subtype'], *s['position'], *s['angles'], *s['transform'])
        state(c['result'])
(root / 'tests/fixtures/herc_aimed_body.bin').write_bytes(out)
print(sum(len(r['cases']) for r in data['cases']), 'composed aiming frames exported')
