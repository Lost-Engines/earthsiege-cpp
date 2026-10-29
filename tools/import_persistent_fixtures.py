#!/usr/bin/env python3
"""Import unmodified original smoke states and physical weapon detachment records."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=Path(sys.argv[1])/'models/persistent-effects'
(root/'tests/fixtures/smoke.bin').write_bytes((source/'native-smoke.bin').read_bytes())
rows=json.loads((source/'native-detached.json').read_text())['cases'];out=bytearray(b'ESDETW01')+struct.pack('<H',len(rows))
def state(v):return struct.pack('<3i3H3h',*v['position'],*v['angles'],*v['velocity'])
for row in rows:
 out+=struct.pack('<hh',row['definition'],row['template'])+bytes.fromhex(row['mount'])+struct.pack('<9hBB3i3i',*row['hardpoint'],*row['centre'])+state(row['initial'])+struct.pack('<H',len(row['steps']))
 for step in row['steps']:out+=state(step)+bytes([step['done']])
(root/'tests/fixtures/detached_weapons.bin').write_bytes(out)
print('Imported original smoke pool traces and',len(rows),'physical weapon detachment traces')
