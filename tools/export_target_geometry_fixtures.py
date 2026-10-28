#!/usr/bin/env python3
"""Pack original component targeting and destruction notifications."""
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1];analysis=root.parent/'earthsiege-reverse/analysis'
rows=json.loads((analysis/'definitions/target-geometry.json').read_text())['worlds'];starts={w['demo']:w['initial_random']for w in json.loads((analysis/'missions/starting-hercs.json').read_text())['worlds']}
out=bytearray(b'ESTARG01')
def put(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
put('H',len(rows))
for w in rows:
 r=starts[f"demo{w['demo']}"];put('H56HBBH',w['demo'],*[x&65535 for x in r[:56]],*r[56:],len(w['actors']))
 for a in w['actors']:
  put('hH',a['kind'],len(a['cases']))
  for c in a['cases']:
   put('3i3H3i3HhH',*c['position'],*c['angles'],*c['source'],*c['aim'],c['selected'],len(c['centres']))
   for part in c['centres']:put('h3i',*part)
   put('3i',*c['fallback'])
(root/'tests/fixtures/target_geometry.bin').write_bytes(out)
rows=json.loads((analysis/'missions/destruction-counters.json').read_text())['cases'];out=bytearray(b'ESCOUNT1');put('H',len(rows))
for c in rows:
 put('HHhhhh',len(c['flags']),c['selected'],c['counter'],c['operation'],c['group_counter'],c['group_operation'])
 for flags in c['flags']:put('BB',*flags)
 put('100h',*c['initial']);put('H',len(c['results']))
 for r in c['results']:put('100h',*r)
(root/'tests/fixtures/destruction_counters.bin').write_bytes(out)
print('Exported target geometry and destruction counter fixtures')
