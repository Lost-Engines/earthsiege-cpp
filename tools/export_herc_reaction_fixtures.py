#!/usr/bin/env python3
"""Pack original HERC retaliation traces."""
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((root.parent/'earthsiege-reverse/work/research/missions/herc-reactions.json').read_text())['cases']
out=bytearray(b'ESHREA01')
def write(*v):out.extend(struct.pack('<'+'i'*len(v),*v))
write(len(rows))
for r in rows:
 for k in ['player','same_group','same_side','attacker','leader_player','leader_target','kind','old','chosen','behavior','cooldown','warning','damage_timer','accumulated','amount','command','ignore_hud_target','designated','clock']:write(r[k])
 write(len(r['events']))
 for e in r['events']:write(['share','voice','select','resume','strategy'].index(e[0]),e[2]if e[0]=='voice'else 0)
 f=r['final']
 for k in ['cooldown','warning','damage_timer','accumulated','selected']:write(f[k])
 write(*f['counts'])
 for k in ['changed','requested','ignore_hud_target','command','designated','clock']:write(f[k])
 write(r['ratio'],*r['after_tick'])
(root/'tests/fixtures/herc_reactions.bin').write_bytes(out)
print(len(rows),'original HERC reactions')
