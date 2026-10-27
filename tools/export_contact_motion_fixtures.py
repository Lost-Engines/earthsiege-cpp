#!/usr/bin/env python3
"""Import original collision-response traces, including both attempted placements."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-contact-motion.json').read_text())
out=bytearray(b'ESBODY02')+struct.pack('<H',len(rows))
def state(value):
 result=bytearray(struct.pack('<3i3H8hH',*value['position'],*value['angles'],*value['movement'],*value['aim'],len(value['poses'])))
 for pose in value['poses']:result+=struct.pack('<3H3h',*pose)
 for matrix in value['matrices']:result+=struct.pack('<9hBB3i',*matrix)
 return result
for row in rows:
 out+=row['name'].encode()+b'\0'+struct.pack('<H'+'h'*len(row['equipment']),len(row['equipment']),*row['equipment'])
 out+=state(row['initial'])+struct.pack('<H',len(row['cases']))
 for case in row['cases']:
  out+=struct.pack('<5h4B',*case['controls'],case['ratio'],*case['hits'],case['collision'],len(case['probes']))
  for probe in case['probes']:out+=struct.pack('<3i3H',*probe)
  out+=state(case['result'])
(root/'tests/fixtures/contact_motion.bin').write_bytes(out)
print(sum(len(row['cases'])for row in rows),'original collision-response updates')
