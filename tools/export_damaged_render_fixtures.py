#!/usr/bin/env python3
"""Import original damaged-body frames and the impacts which produce them."""
import json
import struct
import sys
from pathlib import Path

root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-damaged-render.json').read_text())['cases']
out=bytearray(b'ESDMGR01')+struct.pack('<I',len(rows))
for row in rows:
    for key in ['name','palette']:out+=row[key].encode()+b'\0'
    out+=struct.pack('<H'+'h'*len(row['equipment']),len(row['equipment']),*row['equipment'])
    out+=struct.pack('<3H3iIhhH',*row['angles'],*row['translation'],row['crc32'],row['hit'],row['amount'],len(row['draws']))
    for draw in row['draws']:out+=struct.pack('<2h',draw['bound'],draw['value'])
(root/'tests/fixtures/damaged_render.bin').write_bytes(out)
print(len(rows),'original damaged frames')
