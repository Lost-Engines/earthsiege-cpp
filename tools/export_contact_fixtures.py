#!/usr/bin/env python3
"""Import original actor contact/ramming and moving-body scenery occupancy."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
analysis=Path(sys.argv[1])/'models'
rows=json.loads((analysis/'actor-contact.json').read_text())['cases']
out=bytearray(b'ESCONT01')+struct.pack('<H',len(rows))
for row in rows:
 for i in range(2):
  position=[a+(b if i else 0) for a,b in zip([100000,100000,20000],row['delta'])]
  out+=struct.pack('<3i3H3hi2B',*position,0,0,row['heading'][i],row['radius'][i],row['speed'][i],row['mass'][i],row['heights'][i],1,row['gated'] if i else False)
 events=row['damage_calls']
 out+=struct.pack('<Bi3i',row['hit'],events[0][4] if events else 0,*(events[0][3] if events else [0,0,0]))
(root/'tests/fixtures/actor_contact.bin').write_bytes(out)
rows=json.loads((analysis/'scenery-contact.json').read_text())
out=bytearray(b'ESOCCU01')+struct.pack('<H',len(rows))
for row in rows:
 out+=row['resource'].encode()+b'\0'+struct.pack('<HH',row['root'],len(row['cases']))
 for case in row['cases']:
  out+=struct.pack('<H',len(case['objects']))
  for pos,heading in case['objects']:out+=struct.pack('<3iH',*pos,heading)
  out+=struct.pack('<3ihi',*case['position'],case['radius'],case['height'])
(root/'tests/fixtures/scenery_contact.bin').write_bytes(out)
print('Imported 1400 actor contacts and',sum(len(r['cases'])for r in rows),'scenery contacts')
