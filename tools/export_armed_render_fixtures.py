#!/usr/bin/env python3
"""Import complete original armed and empty-mount HERC frames."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-armed-render.json').read_text())['cases']
out=bytearray(b'ESARMED1')+struct.pack('<I',len(rows))
for row in rows:
 for key in ['name','texture','palette']:out+=row[key].encode()+b'\0'
 out+=struct.pack('<H'+'h'*len(row['equipment']),len(row['equipment']),*row['equipment'])
 out+=struct.pack('<3H3iI5h',*row['angles'],*row['translation'],row['crc32'],*(row['pose']or[-1,0,0,0,0]))
(root/'tests/fixtures/armed_render.bin').write_bytes(out)
print(len(rows),'complete original assembly frames')
