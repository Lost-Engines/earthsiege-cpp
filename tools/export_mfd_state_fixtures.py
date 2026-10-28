#!/usr/bin/env python3
"""Import original parent MFD state and child-dispatch traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/mfd-state.json').read_text())
def state(r):return struct.pack('<4i7BIhhB13h',r['mode'],r['range'],r['selected'],r['choice'],r['dirty'],*r['refresh'],r['deadline'],r['previous'],r['automatic'],r['guard'],*r['faces'])
out=bytearray(b'ESMFD001')+struct.pack('<I',len(data['cases']))+state(data['initial'])
for r in data['cases']:
    flags=sum(1<<i for i,k in enumerate(['manual','hidden','external','target','range'])if r[k])
    out+=struct.pack('<IiBbBB',r['now'],r['mode'],flags,r['scroll'],r['disabled'],len(r['calls']))
    for kind,page in r['calls']:out+=struct.pack('<BB',kind=='full',page)
    out+=state(r['state'])
(root/'tests/fixtures/mfd_state.bin').write_bytes(out)
print(len(data['cases']),'MFD state transitions exported')
