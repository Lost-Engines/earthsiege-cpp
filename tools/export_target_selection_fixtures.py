#!/usr/bin/env python3
"""Export original target ranking inputs, decisions and shared random states."""
import json
from pathlib import Path
import struct
import sys
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1]) / 'missions/target-selection.json').read_text())['cases']
with (root / 'tests/fixtures/target_selection.bin').open('wb') as out:
    def pack(fmt, *v): out.write(struct.pack('<' + fmt, *v))
    def optional(v): pack('h', -1 if v is None else v)
    def random(v): pack('56H2B', *[x & 65535 for x in v[:56]], *v[56:])
    out.write(b'ESTARG01'); pack('I', len(rows))
    for r in rows:
        pack('Bh', r['flags'], r['mission']); optional(r['hud_target']); optional(r['selected'])
        g = r['group']; pack('hH', g['order'], g['leader']); optional(g['representative'])
        pack('H' + 'H' * len(g['targets']), len(g['targets']), *g['targets'])
        random(r['initial_random']); random(r['final_random'])
        pack('H', len(r['actors']))
        for a in r['actors']:
            pack('3iHhBh', *a['position'], a['heading'], a['kind'], a['side'], a['strength'])
            out.write(bytes(a['known']))
            pack('9B', *[a[k] for k in ['seen','dead','removed','disabled','flag9f','flag9a','player','ignore_hud_target','controller'][:-1]], a['controller'] is not None)
            pack('B', bool(a['controller'])); optional(a['target']); pack('h', a['attackers'])
print(len(rows), 'original target decisions imported')
