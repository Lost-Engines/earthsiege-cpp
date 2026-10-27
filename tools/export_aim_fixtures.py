#!/usr/bin/env python3
"""Import original physical torso/elevation control references."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-aim.json').read_text())['cases']
out=bytearray(b'ESAIM001')+struct.pack('<I',len(rows))
for r in rows:out+=r['model'].encode()+b'\0'+struct.pack('<B6h',r['axis']=='pitch',r['ratio'],r['input'],r['old_rate'],r['old_angle'],*r['result'])
(root/'tests/fixtures/aim_control.bin').write_bytes(out)
print(len(rows),'original aiming control cases imported')
