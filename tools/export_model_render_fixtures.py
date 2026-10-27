#!/usr/bin/env python3
"""Import original complete model drawing references from the research tree."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'models'
rows=json.loads((source/'native-model-render.json').read_text())['cases']
if (source/'native-animated-render.json').exists():rows+=json.loads((source/'native-animated-render.json').read_text())['cases']
out=bytearray(b'ESMODEL2')+struct.pack('<I',len(rows))
for row in rows:
 for field in ['resource','texture','palette']:out+=(row[field]or'').encode()+b'\0'
 out+=struct.pack('<H3H3iI',row['root'],*row['angles'],*row['translation'],row['crc32'])
 out+=struct.pack('<5h',*row.get('pose',[-1,0,0,0,0]))
(root/'tests/fixtures/model_render.bin').write_bytes(out)
print(len(rows),'complete model fixtures exported')
