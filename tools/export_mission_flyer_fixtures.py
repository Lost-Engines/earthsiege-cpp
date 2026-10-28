#!/usr/bin/env python3
"""Export complete original mission flyer starts."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];d=json.loads((Path(sys.argv[1])/'missions/starting-flyers.json').read_text());out=bytearray(b'ESFSTART')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
pack('H',len(d['worlds']))
for w in d['worlds']:
 out.extend(w['demo'].upper().encode()+b'\0');pack('H',len(w['actors']))
 for a in w['actors']:
  pack('HHBBBB3i3H3h3h',a['source'],a['group'],a['side'],a['waiting'],a['dead'],a['detectable'],*a['position'],*a['angles'],*a['velocity'],*a['rates'])
  for name in ['internal','armour','present']:
   v=a['damage'][name];pack('H',len(v));pack('h'*len(v),*v)
  pack('H',len(a['cells']));pack('H'*len(a['cells']),*a['cells'])
(root/'tests/fixtures/mission_flyers.bin').write_bytes(out)
print(sum(len(w['actors'])for w in d['worlds']),'mission flyer starts exported')
