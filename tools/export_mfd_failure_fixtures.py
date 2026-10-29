#!/usr/bin/env python3
"""Import combined original parent/failure transitions, including UI RNG."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/mfd-failure-state.json').read_text())
def state(r):return struct.pack('<4i7BIhhB13h',r['mode'],r['range'],r['selected'],r['choice'],r['dirty'],*r['refresh'],r['deadline'],r['previous'],r['automatic'],r['guard'],*r['faces'])
def failure(r):return struct.pack('<HII2B',*r['failure'])
out=bytearray(b'ESMFDF01')+struct.pack('<I',len(data['cases']))+state(data['initial'])+failure(data['initial'])+struct.pack('<56h2B',*data['initial_random'])
for r in data['cases']:
 flags=sum(1<<i for i,k in enumerate(['manual','hidden','external','target','range'])if r[k])
 out+=struct.pack('<IiBbBBB',r['now'],r['mode'],flags,r['scroll'],r['disabled'],len(r['calls']),r['severity'])
 for kind,page in r['calls']:
  out+=struct.pack('<3B',kind!='update',r['state']['mode']if kind=='failure'else page,max(1,page) if kind=='failure'else 0)
 out+=state(r['state'])+failure(r['state'])+struct.pack('<B',bool(r['sounds']))
 assert r['sounds'] in [[],[28]]
out+=struct.pack('<56h2B',*data['final_random'])
(root/'tests/fixtures/mfd_failure.bin').write_bytes(out)
print(len(data['cases']),'MFD failure transitions exported')
