#!/usr/bin/env python3
"""Export original travel, flee, skirt and ram decisions with scratch/RNG state."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-auxiliary.json').read_text())
out = bytearray(b'ESHAU001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
pack('I', len(data['cases']))
for r in data['cases']:
    kind = ['travel', 'flee', 'skirt', 'ram'].index(r['kind'])
    pack('B6i2H2hhi6B2Bh4B', kind, *r['position'], *r['target'], r['heading'], r['target_heading'], r['view'], r['target_view'], r['ratio'], r['timer'], *r['flags'], r['old_target'], r['selected'], r['command'], r['detectable'], r['persistent'], r['threat'], r['turn_choice'])
    pack('3B3iiBHBh3ih56h2B', r['previous'], r['initialized'], r['left'], *r['obstacle'], r['distance'], r['blocked'], r['stagger'], r['burst'], r['burst_timer'], *r['advance_delta'], r['advance_turn'], *r['random'])
    f = r['final']; pack('HB', f['result'], len(f['events']))
    for e in f['events']: pack('4i', ['move', 'aim', 'centre', 'route', 'select', 'blocked', 'enter', 'advance'].index(e[0]), *e[1:], *([0] * (4 - len(e))))
    pack('BhBhB3iHi56h2B', f['selected'], f['count'], f['changed'], f['command'], f['detectable'], *f['position'], f['heading'], f['timer'], *f['random'])
    if kind == 0: pack('B', f['threat'])
    elif kind == 1: pack('Bh', f['turn_choice'], f['last_direction'])
    elif kind == 2: pack('h2BiBH', f['previous'], f['initialized'], f['left'], f['distance'], f['obstruction'], f['stagger'])
    else: pack('Bh', f['burst'], f['burst_timer'])
(root / 'tests/fixtures/herc_auxiliary.bin').write_bytes(out)
print(len(data['cases']), 'original auxiliary decisions exported')
