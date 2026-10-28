#!/usr/bin/env python3
"""Import original shared-pool projectile lifetimes, launch and hit ordering."""
import json
from pathlib import Path
import struct
import sys
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1]) / 'definitions/projectile-world.json').read_text())['cases']
with (root / 'tests/fixtures/projectile_world.bin').open('wb') as out:
    def pack(fmt, *v): out.write(struct.pack('<' + fmt, *v))
    def rng(v): pack('56H2B', *[x & 65535 for x in v[:56]], *v[56:])
    def words(v): pack('H' + 'H' * len(v), len(v), *v)
    out.write(b'ESPRWR01'); pack('H', len(rows))
    for row in rows:
        pack('h', row['ratio']); rng(row['initial_random']); pack('H', len(row['steps']))
        for frame in row['steps']:
            pack('iH', frame['hit'], frame['hits']); words(frame['order']); words(frame['retired'])
            pack('H', len(frame['requests']))
            for v in frame['requests']:
                pack('BHhi3i3HH', v['kind'], v['definition'], v['charge'], v['range'], *v['position'], *v['angles'], v['slot'])
            pack('H', len(frame['live']))
            for v in frame['live']:
                pack('HB', v['slot'], v['beam'])
                if v['beam']: pack('H', v['points'])
                else:
                    pack('hB3i3H3h', v['kind'], v['subtype'], *v['position'], *v['angles'], v['speed'], v['age'], v['remaining'])
                    words(v['cells'])
            rng(frame['random'])
print(sum(len(r['steps']) for r in rows), 'original mixed projectile frames imported')
