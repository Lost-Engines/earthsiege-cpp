#!/usr/bin/env python3
"""Export original flyer route and formation navigation traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];d=json.loads((Path(sys.argv[1])/'definitions/flyer-navigation.json').read_text());out=bytearray(b'ESFNAV01')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def state(s):pack('3i3H3h3hhhB',*s['position'],*s['angles'],*s['velocity'],*s['rates'],s['strength'],s['timer'],s['obstacle'])
pack('H',len(d['cases']))
for r in d['cases']:
 pack('B',r['kind']=='formation');state(r['initial']);pack('h',r['speed'])
 if r['kind']=='route':
  pack('H',len(r['points']))
  for p in r['points']:pack('3i',*p)
  pack('Bhh',r['closed'],r['cursor'],r['next_cursor'])
 else:state(r['leader']);pack('3i',*r['offset'])
 state(r['state']);pack('h',r['requested'])
(root/'tests/fixtures/flyer_navigation.bin').write_bytes(out)
print(len(d['cases']),'flyer navigation cases exported')
