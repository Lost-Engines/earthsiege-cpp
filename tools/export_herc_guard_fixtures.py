#!/usr/bin/env python3
"""Export original guard and drive-off decisions, preserving ordered boundaries."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
expansion = len(sys.argv) > 2 and sys.argv[2] == '--expansion'
name = 'expansion-herc-guard' if expansion else 'herc-guard'
data = json.loads((Path(sys.argv[1]) / ('missions/' + name + '.json')).read_text())
out = bytearray(b'ESHGD002' if expansion else b'ESHGD001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
pack('I', len(data['cases']))
for r in data['cases']:
    pack('B', r['mode'] == 'drive')
    if expansion: pack('2B', r['expansion'], r['weapons_free'])
    for actor in r['actors']:
        pack('3iH4Bh', *actor['position'], actor['heading'], actor['eligible'], actor['dead'], actor['disabled'], actor['unavailable'], actor['attackers'])
    pack('3i5h5BhH3Bh', *r['anchor'], r['command'], r['timer'], r['ratio'], r['selected'], r['designated'], r['leader_player'], r['radar'], r['persistent'], r['detectable'], r['objective'], r['component'], r['stagger'], r['blocked'], r['faceoff'], r['completed'], r['target_view'])
    pack('BH', len(r['events']), r['result'])
    for e in r['events']:
        pack('4i', ['eligible', 'navigate', 'centre', 'move_to', 'move', 'component', 'enter', 'obstruction', 'faceoff', 'completed', 'aim'].index(e[0]), *e[1:], *([0] * (4 - len(e))))
    f = r['final']
    pack('11hB3hBBiH', f['selected'], f['designated'], *f['counts'], f['changed'], f['timer'], f['component'], f['tolerance'], f['detectable'], f['behavior'], f['clock'], f['stagger'])
(root / ('tests/fixtures/' + name.replace('-', '_') + '.bin')).write_bytes(out)
print(len(data['cases']), 'original guard and drive-off decisions exported')
