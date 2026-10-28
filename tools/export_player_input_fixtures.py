#!/usr/bin/env python3
"""Import original player control-routing captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'interface/player-input.json').read_text())['cases']
out=bytearray(b'ESPINP01')+struct.pack('<I',len(rows))
events=['camera','move','flyer','yaw','pitch','track','centre','fire','target']
for r in rows:
    values=[*(r[k]for k in ['external','guided','active','disabled','tracking','target','dead']),*r['movement'],*r['aiming'],*r['camera'],*r['mouse'],*(r[k]for k in ['left','right','fire','enter','release','release_action']),*r['position'],*r['target_position'],*r['target_offset'],*(r[k]for k in ['reset','command','result'])]
    out.extend(struct.pack('<33i',*values))
    out.extend(struct.pack('<H',len(r['events'])))
    for e in r['events']:out.extend(struct.pack('<HH'+'i'*(len(e)-1),events.index(e[0]),len(e)-1,*e[1:]))
    s=r['state'];out.extend(struct.pack('<11i',s['latched'],*s['origin'],*s['response'],*(s[k]for k in ['centred','enter','reset_aim','stop','throttle_changed','flyer_held'])))
(root/'tests/fixtures/player_input.bin').write_bytes(out)
print(len(rows),'original player routing updates')
