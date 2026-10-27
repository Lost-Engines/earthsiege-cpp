#!/usr/bin/env python3
"""Import complete original HERC top-level detail frames."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-actor-detail-render.json').read_text())['cases']
out=bytearray(b'ESDETAIL')+struct.pack('<I',len(rows))
for row in rows:
 for key in ['name','texture','palette']:out+=row[key].encode()+b'\0'
 out+=struct.pack('<H'+'h'*len(row['equipment']),len(row['equipment']),*row['equipment'])
 out+=struct.pack('<3H3iI3h',*row['angles'],*row['translation'],row['crc32'],row['scale'],row['bias'],row['detail'])
(root/'tests/fixtures/actor_detail.bin').write_bytes(out)
print(len(rows),'original HERC detail frames')
