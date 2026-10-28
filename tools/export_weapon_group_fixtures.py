#!/usr/bin/env python3
"""Import original weapon group button captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/weapon-groups.json').read_text());rows=data['cases']
out=bytearray(b'ESWPNG01')+struct.pack('<I',len(rows))
for r in rows:
 out+=r['chassis'].encode()+b'\0'+struct.pack('<6BI',r['group'],*r['faces'],r['crc32'])
 out+=struct.pack('<20i',*(v for rect in r['bounds']for v in rect))
out+=struct.pack('<I',len(data['input_cases']))
for r in data['input_cases']:
 out+=struct.pack('<12B',r['group'],r['requested'],r['keyboard'],r['tracking'],r['wanted_tracking'],r['link'],*r['result'])
(root/'tests/fixtures/weapon_groups.bin').write_bytes(out)
print(len(rows),'weapon group frames exported')
