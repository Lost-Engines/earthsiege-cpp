#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
j=json.loads((Path(sys.argv[1])/'cockpits/tactical-targets.json').read_text());b=bytearray(b'ESTTGT01')
def p(fmt,*v):b.extend(struct.pack('<'+fmt,*v))
p('4iI',*j['bounds'],len(j['cases']))
for r in j['cases']:
 p('6iBiiH',*r['camera'],*r['origin'],r['side'],r['previous'],r['result'],len(r['contacts']))
 for c in r['contacts']:p('3i5B',*c['position'],c['side'],c['dead'],c['selectable'],c['detected'],c['known'])
(Path(__file__).resolve().parents[1]/'tests/fixtures/tactical_targets.bin').write_bytes(b)
