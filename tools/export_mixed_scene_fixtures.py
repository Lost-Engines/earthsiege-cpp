#!/usr/bin/env python3
"""Import original scenery plus player-roster actor frames."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'worlds/native-mixed-scene.json').read_text())
out=bytearray(b'ESMIXED1')+struct.pack('<H',len(rows))
for row in rows:
 out+=row['demo'].upper().encode()+b'\0'+struct.pack('<10hH',*row['header'],len(row['actors']))
 for actor in row['actors']:out+=struct.pack('<2h3i3H',actor['chassis'],actor['native_class'],*actor['position'],*actor['angles'])
 out+=struct.pack('<H',len(row['cases']))
 for case in row['cases']:out+=struct.pack('<3i3HIH'+'H'*len(case['order']),*case['position'],*case['angles'],case['crc32'],len(case['order']),*case['order'])
(root/'tests/fixtures/mixed_scene.bin').write_bytes(out)
print(sum(len(row['cases'])for row in rows),'original mixed scene frames')
