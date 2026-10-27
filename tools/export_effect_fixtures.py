#!/usr/bin/env python3
"""Import original effect factories, shared RNG states and combined animation."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];analysis=Path(sys.argv[1])
created=json.loads((analysis/'models/effect-creation.json').read_text());updated=json.loads((analysis/'models/effect-updates.json').read_text())
out=bytearray(b'ESFX0001')+struct.pack('<3I3i',len(created['explosions']),len(created['bursts']),len(updated['cases']),1234,-5678,9012)
def rng(s):return struct.pack('<56h2B',*s)
def shorts(s):return struct.pack('<H'+'h'*len(s),len(s),*s)
def state(s):return struct.pack('<h',s['timer'])+shorts(s['cells'])+struct.pack('<2h',s['secondary']if s['secondary']is not None else -1,s['intensity']if s['intensity']is not None else -1)
for row in created['explosions']:
    r=row['result'];out+=struct.pack('<H',row['definition'])+rng(row['initial_random'])
    out+=struct.pack('<h3H3ih',r['kind'],*r['angles'],*r['position'],r['timer'])+shorts(r['cells'])
    out+=struct.pack('<B2h',r['secondary'],r['light']if r['light']is not None else -1,r['sounds'][0][1])+rng(r['random'])
for row in created['bursts']:
    out+=struct.pack('<H',row['group'])+rng(row['initial_random'])+struct.pack('<H',len(row['particles']))
    for p in row['particles']:out+=struct.pack('<h3i3H3h',p['model'],*p['position'],*p['angles'],*p['velocity'])
    out+=rng(row['final_random'])
for row in updated['cases']:
    out+=struct.pack('<HhH',row['definition'],row['ratio'],row['secondary_count'])+shorts(row['frame_counts'])+state(row['initial'])+struct.pack('<H',len(row['states']))
    for s in row['states']:out+=state(s)+struct.pack('<B',s['done'])
(root/'tests/fixtures/effects.bin').write_bytes(out)
print(len(created['explosions']),'explosions,',len(created['bursts']),'bursts,',sum(len(r['states'])for r in updated['cases']),'animation updates')
