#!/usr/bin/env python3
"""Import original combined HERC movement/aiming/pose traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
damaged='--damaged' in sys.argv
requested='--requested' in sys.argv or damaged
rows=json.loads((Path(sys.argv[1])/'models'/('native-herc-damaged.json' if damaged else 'native-herc-request.json' if requested else 'native-herc-body.json')).read_text())
out=bytearray(b'ESBODY04' if damaged else b'ESBODY03' if requested else b'ESBODY01')+struct.pack('<H',len(rows))
def state(value):
 result=bytearray(struct.pack('<3i3H8hH',*value['position'],*value['angles'],*value['movement'],*value['aim'],len(value['poses'])))
 for pose in value['poses']:result+=struct.pack('<3H3h',*pose)
 for matrix in value['matrices']:result+=struct.pack('<9hBB3i',*matrix)
 return result
for row in rows:
 out+=row['name'].encode()+b'\0'+struct.pack('<H'+'h'*len(row['equipment']),len(row['equipment']),*row['equipment'])
 out+=state(row['initial'])+struct.pack('<H',len(row['cases']))
 for case in row['cases']:
  out+=struct.pack('<5h',*case['controls'],case['ratio'])
  if damaged:
   c=case['condition'];out+=struct.pack('<3BiBiBB',c['disabled'],c['light'],c['heavy'],c['recovery'],c['reverse'],case['recovery'],case['collapsed'],len(case['events']))
   for event in case['events']:
    if event[0]=='collapse':out+=struct.pack('<B2h',1,*event[1:])
    else:out+=struct.pack('<Bh',0,event[1])
  out+=state(case['result'])
(root/'tests/fixtures'/('herc_damaged.bin' if damaged else 'herc_request.bin' if requested else 'herc_body.bin')).write_bytes(out)
print(sum(len(row['cases'])for row in rows),'original combined HERC updates')
