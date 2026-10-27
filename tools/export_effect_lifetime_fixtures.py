#!/usr/bin/env python3
"""Export original debris motion and fixed-pool allocation/reclamation references."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];analysis=Path(sys.argv[1])
rows=json.loads((analysis/'models/debris-motion.json').read_text())['cases'];out=bytearray(b'ESDEBR01')+struct.pack('<I',len(rows))
def state(s):return struct.pack('<3i3H3h',*s['position'],*s['angles'],*s['velocity'])
for row in rows:
 out+=struct.pack('<h',row['ratio'])+state(row['initial'])+struct.pack('<H',len(row['states']))
 for s in row['states']:out+=state(s)+struct.pack('<B',s['done'])
(root/'tests/fixtures/debris_motion.bin').write_bytes(out)
rows=json.loads((analysis/'models/object-pools.json').read_text())['cases'];out=bytearray(b'ESPOOL01')+struct.pack('<H',len(rows))
def indices(a):return struct.pack('<H'+'H'*len(a),len(a),*a)
for row in rows:
 out+=struct.pack('<HH',row['capacity'],len(row['checks']))
 for r in row['checks']:out+=struct.pack('<BhH',r['operation'],r['index'],len(r['free']))+indices(r['active'])+indices(r['oldest'])+indices(r['pending'])+indices(r['destroyed'])
(root/'tests/fixtures/object_pools.bin').write_bytes(out)
