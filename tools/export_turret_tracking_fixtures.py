#!/usr/bin/env python3
"""Export original articulated turret target tracking."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=json.loads((Path(sys.argv[1])/'definitions/turret-tracking.json').read_text());out=bytearray(b'ESTAIM01')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
pack('H',len(data['cases']))
for r in data['cases']:
 pack('3iHH',*r['position'],r['heading'],len(r['steps']))
 for s in r['steps']:
  pack('3i7hH',*s['target'],*s['errors'],*s['rates'],*s['angles'],len(s['matrices']))
  for m in s['matrices']:pack('9hBB3i',*m)
(root/'tests/fixtures/turret_tracking.bin').write_bytes(out);print(sum(len(r['steps'])for r in data['cases']),'turret tracking steps exported')
