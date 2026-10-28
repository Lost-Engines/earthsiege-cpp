#!/usr/bin/env python3
"""Export original base collision, damage, animation and destruction traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=json.loads((Path(sys.argv[1])/'definitions/ground-combat.json').read_text());out=bytearray(b'ESGCOMB1')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def state(s):
 pack('hBBhhh',s['damage'],s['dead'],s['wreck'],s['bursts'],s['burst_timer'],s['cell_timer'])
 pack('H',len(s['cells']));pack('H'*len(s['cells']),*s['cells']);pack('H',len(s['matrices']))
 for m in s['matrices']:pack('9hBB3i',*m)
 pack('56h2B',*s['random'])
def events(values):
 pack('H',len(values))
 for v in values:pack('5i',*(v+[0]*(5-len(v))))
pack('H',len(data['cases']))
for r in data['cases']:
 pack('hB',r['type'],r['side']);state(r['initial'])
 for key in ['animation','destruction']:
  pack('H',len(r[key]))
  for o in r[key]:
   pack('Bh',o['op']=='damage',o.get('amount',o.get('ratio')));state(o['state']);events(o['events'])
 pack('H',len(r['rays']))
 for o in r['rays']:
  pack('B',o['stage'])
  pack('3iHihhBhBi',*o['position'],o['heading'],o['reach'],o['radius'],o['damage'],o['attacker_side'],o['difficulty'],o['override'],o['result']);state(o['state']);events(o['events'])
(root/'tests/fixtures/ground_combat.bin').write_bytes(out);print(len(data['cases']),'base combat cases exported')
