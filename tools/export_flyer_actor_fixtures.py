#!/usr/bin/env python3
"""Export original skimmer damage and complete VGA frame checks."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
d=json.loads((Path(sys.argv[1])/'definitions/flyer-actors.json').read_text());out=bytearray(b'ESFLYER1')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def state(s):
 for name in ['internal','armour','present','cells']:
  pack('H',len(s[name]));pack('h'*len(s[name]),*s[name])
 pack('3B3i3H3h3h',s['dead'],s['removed'],s['detectable'],*s['position'],*s['angles'],*s['velocity'],*s['rates'])
 pack('56h2B',*s['random'])
def events(rows):
 pack('H',len(rows))
 for r in rows:pack('5i',*(r+[0]*(5-len(r))))
state(d['initial']);pack('H',len(d['frames']))
for f in d['frames']:pack('3i3HI',*f['position'],*f['angles'],f['crc32'])
pack('H',len(d['damage']))
for r in d['damage']:pack('hB',r['amount'],r['result']);state(r['state']);events(r['events'])
pack('H',len(d['rays']))
for r in d['rays']:
 pack('B3iHihhBhBBi',r['stage'],*r['position'],r['heading'],r['reach'],r['radius'],r['damage'],r['attacker_side'],r['difficulty'],r['override'],r['locked'],r['result']);state(r['state']);events(r['events'])
(root/'tests/fixtures/flyer_actors.bin').write_bytes(out)
print(len(d['frames']),'frames;',len(d['rays']),'rays exported')
