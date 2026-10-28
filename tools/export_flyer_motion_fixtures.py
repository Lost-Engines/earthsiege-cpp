#!/usr/bin/env python3
"""Export original flyer steering, motion and obstacle-query traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];d=json.loads((Path(sys.argv[1])/'definitions/flyer-motion.json').read_text());out=bytearray(b'ESFMOVE1')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def state(s):pack('3i3H3h3hhhB',*s['position'],*s['angles'],*s['velocity'],*s['rates'],s['strength'],s['timer'],s['obstacle'])
pack('H',len(d['cases']))
for row in d['cases']:
 state(row['initial']);pack('H',len(row['steps']))
 for s in row['steps']:
  pack('B4i',s['op'],*s['args']);state(s['steered']);pack('h',s['ratio'])
  for v in s['distances']:pack('Bi',v is not None,v or 0)
  pack('B',len(s['probes']))
  for q in s['probes']:pack('B6i',*q)
  state(s['state'])
(root/'tests/fixtures/flyer_motion.bin').write_bytes(out)
print(sum(len(r['steps'])for r in d['cases']),'flyer motion steps exported')
