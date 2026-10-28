#!/usr/bin/env python3
"""Import original cockpit target-cycle captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'interface/player-targeting.json').read_text())['cases']
out=bytearray(b'ESPTAR01')+struct.pack('<I',len(rows))
for r in rows:
    out.extend(struct.pack('<12iH',*r['origin'],*(r[k]for k in ['heading','torso','side','weather','seen_range','previous','selected','stored','changed']),len(r['candidates'])))
    for c in r['candidates']:out.extend(struct.pack('<8i',*c['position'],*(c[k]for k in ['side','seen','known','dead','disabled'])))
(root/'tests/fixtures/player_targeting.bin').write_bytes(out)
print(len(rows),'original cockpit target cycles')
