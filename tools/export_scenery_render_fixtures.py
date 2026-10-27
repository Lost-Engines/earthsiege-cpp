#!/usr/bin/env python3
"""Import complete original static-scene references from the research tree."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'worlds/native-scenery-render.json').read_text())
out=bytearray(b'ESSCENE1')+struct.pack('<H',len(rows))
for row in rows:
 out+=struct.pack('<10hH',*row['header'],len(row['cases']))
 for case in row['cases']:
  out+=struct.pack('<3i3HIH',*case['position'],*case['angles'],case['crc32'],len(case['order']))
  out+=struct.pack('<'+'H'*len(case['order']),*case['order'])
(root/'tests/fixtures/scenery_render.bin').write_bytes(out)
print(sum(len(r['cases'])for r in rows),'complete scenery fixtures exported')
