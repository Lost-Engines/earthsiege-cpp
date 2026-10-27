#!/usr/bin/env python3
"""Import every node from independently checked original cross-detail bindings."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'models'
rows=json.loads((source/'runtime-bindings.js').read_text().split('=',1)[1].rstrip(';\n'))
out=bytearray(b'ESBIND01')+struct.pack('<H',len(rows));nodes=0
for name,row in rows.items():
 out+=name.encode()+b'\0'+struct.pack('<H',len(row['part_ids']))+bytes(row['part_ids'])
 shapes=json.loads((source/row['url']).read_text());values=[]
 def visit(o):
  values.append(o.get('transform',-1))
  values.extend(o.get('bsp_transforms',[]))
  for child in o.get('parts',[]):visit(child)
 for shape in shapes:visit(shape)
 nodes+=len(values)
 out+=struct.pack('<I'+'h'*len(values),len(values),*[((v+32768)%65536)-32768 for v in values])
(root/'tests/fixtures/model_bindings.bin').write_bytes(out)
print(len(rows),'original binding sets;',nodes,'part/node transforms')
