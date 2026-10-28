#!/usr/bin/env python3
"""Export original attack strategy, target counts, callbacks and RNG."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-strategy.json').read_text())
out = bytearray(b'ESHST001')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
pack('I', len(data['cases']))
for r in data['cases']:
    pack('B2hB2hBh2B2hH', r['observer'], r['order'], r['command'], r['requested'], r['old'], r['chosen'], r['retreat'], r['component'], r['radar'], r['detectable'], r['ecm'], r['limit'], r['stagger'])
    for a in r['actors']: pack('4BhB', a['behavior'], a['dead'], a['disabled'], a['player'], a['strength'], a['kind'])
    pack('56h2B', *r['random']); pack('B', len(r['events']))
    for event in r['events']: pack('Bh', ['select', 'follower', 'retreat', 'component', 'resume', 'strength', 'enter'].index(event[0]), event[1] if len(event) > 1 else 0)
    s = r['result']
    pack('hB10h2BhBiHB56h2B', s['selected'], s['changed'], *s['counts'], s['requested'], s['detectable'], s['component'], s['behavior'], s['clock'], s['stagger'], s['first'], *s['random'])
(root / 'tests/fixtures/herc_strategy.bin').write_bytes(out)
print(len(data['cases']), 'original strategies exported')
