#!/usr/bin/env python3
"""Export original gun-turret firing policy, articulation and shot requests."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=json.loads((Path(sys.argv[1])/'definitions/turret-firing.json').read_text());out=bytearray(b'ESTFIRE1')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
pack('Hhh',len(data['cases']),data['projectile'],data['lead_speed'])
for r in data['cases']:
 pack('3iHH',*r['position'],r['heading'],len(r['steps']))
 for v in r['steps']:
  i=v['input'];s=v['state'];pack('hB3i3h3Hh',v['ratio'],i['available'],*i['position'],*i['offset'],*i['angles'],i['speed'])
  pack('B5hH4h',s['target'],s['target_timer'],s['burst_timer'],s['burst_phase'],s['refire_timer'],s['cell_timer'],*s['cells'],*s['rates'],*s['angles'])
  for m in s['matrices']:pack('9hBB3i',*m)
  pack('H',len(v['events']))
  for event in v['events']:
   e=[int(event[0]=='shot'),*event[1:]];pack('8i',*(e+[0]*(8-len(e))))
(root/'tests/fixtures/turret_firing.bin').write_bytes(out);print(sum(len(r['steps'])for r in data['cases']),'turret firing steps exported')
