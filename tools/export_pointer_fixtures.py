#!/usr/bin/env python3
"""Import original cockpit mouse-queue observations."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/pointer-controls.json').read_text())['cases']
out=bytearray(b'ESPTR001')+struct.pack('<I',len(rows))
for row in rows:
 out+=row['chassis'].encode()+b'\0'+struct.pack('<I',len(row['cases']))
 for c in row['cases']:
  out+=struct.pack('<2iBhB3B4i3i',*c['mouse'],c['buttons'],c['selected'],c['dragging'],*c['faces'],*c['thumb'],c['boundary'],c['request'],c['balance'])
(root/'tests/fixtures/pointer_controls.bin').write_bytes(out)
print(sum(len(row['cases']) for row in rows),'original pointer cycles exported')
