#!/usr/bin/env python3
"""Import original spatial trigger, counter and audience comparisons."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'missions/triggers.json').read_text())['cases']
out=bytearray(b'ESTRIG01')+struct.pack('<H',len(rows))
for r in rows:
 kind=r['type']
 if kind=='area':out+=struct.pack('<Bh9iiB',0,r['kind'],*r['origin'],*r['second_point'],*r['position'],r['radius'],r['result'])
 elif kind=='action':out+=struct.pack('<B3hHh',1,r['counter'],r['operation'],r['initial'],r['calls'],r['result'])
 elif kind=='latch':out+=struct.pack('<BiBh',2,r['x'],r['result'],r['counter'])
 else:out+=struct.pack('<B6B',3,r['mode'],r['inside'],r['side'],r['kind'],r['waiting'],r['result'])
(root/'tests/fixtures/triggers.bin').write_bytes(out)
print(len(rows),'original trigger cases')
