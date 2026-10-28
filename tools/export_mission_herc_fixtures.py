#!/usr/bin/env python3
"""Export original full mission HERC starting states."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'missions/starting-hercs.json').read_text())
out=bytearray(b'ESROSTR1')+struct.pack('<H',len(data['worlds']))
def random(v):return struct.pack('<56H2B',*[x&65535 for x in v[:56]],*v[56:])
def shorts(v):return struct.pack('<H'+'h'*len(v),len(v),*v)
flags=['dead','disabled','movement_light','movement_heavy','reactor_light','reactor_heavy','salvage_excluded','collapsed','detectable','waiting']
for w in data['worlds']:
 out+=w['demo'].upper().encode()+b'\0'+random(w['initial_random'])+random(w['final_random'])+struct.pack('<H',len(w['actors']))
 for a in w['actors']:
  out+=struct.pack('<HHBBB3i3H',a['source'],a['group'],a['side'],a['player_roster'],a['player'],*a['position'],*a['angles'])
  out+=b''.join(shorts(a['damage'][k])for k in ['internal','armour','present','cells'])
  out+=struct.pack('<10B',*[a['flags'][k]for k in flags])
(root/'tests/fixtures/mission_hercs.bin').write_bytes(out)
print(len(data['worlds']),'mission rosters exported')
