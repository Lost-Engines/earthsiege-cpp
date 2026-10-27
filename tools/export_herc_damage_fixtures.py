#!/usr/bin/env python3
"""Export complete original HERC impact damage, effect ordering and shared RNG state."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/herc-impacts.json').read_text())
out=bytearray(b'ESHCOM01')+struct.pack('<H',len(data['checks']))
flags=['dead','disabled','movement_light','movement_heavy','reactor_light','reactor_heavy','detectable','shield_warning']
def shorts(v):return struct.pack('<H'+'h'*len(v),len(v),*v)
def state(s):
 return struct.pack('<4h8Bhi56H2B',*s['shields'],*[s['flags'][k]for k in flags],s['ecm_timer'],s['damage_received'],*[x&65535 for x in s['random'][:56]],*s['random'][56:])+b''.join(shorts(s[k])for k in ['internal','armour','present','cells'])
for field in data['checks']:
 out+=field['name'].encode()+b'\0'+shorts(field['equipment'])+struct.pack('<12hH',*field['effects'],len(field['cases']))
 for r in field['cases']:
  out+=struct.pack('<3iHi5hBBhBBB',*r['position'],r['heading'],r['reach'],r['radius'],r['armour_damage'],r['shield_damage'],r['splash'],r['signature'],r['player'],r['attacker_side'],r['difficulty'],r['override'],r['side'],r['invulnerable'])
  out+=state(r['initial'])+state(r['final'])+struct.pack('<i',r['result'])
  events=[]
  for e in r['events']:
   if e[0]=='damage':events.append([0,*e[1:]])
   elif e[0]=='explosion':events.append([1,*e[1:]])
   elif e[0]=='smoke':events.append([2,e[1]//10,*e[2:]])
   elif e[0]=='speech':
    assert e[1]==8,e
    events.append([3])
   elif e[0]=='cockpit':events.append([4,e[2]])
   elif e[0]=='attached_smoke':events.append([5,*e[2:]])
   elif e[0]=='weapon_ejected':events.append([6,*e[1:]])
   elif e[0]=='salvage':events.append([7,*e[1:]])
   elif e[0]!='radial':raise ValueError(e)
  out+=struct.pack('<H',len(events))
  for e in events:out+=struct.pack('<B'+'i'*len(e),len(e),*e)
(root/'tests/fixtures/herc_damage.bin').write_bytes(out)
print(sum(len(r['cases'])for r in data['checks']),'complete HERC impact fixtures')
