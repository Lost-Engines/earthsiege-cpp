#!/usr/bin/env python3
"""Pack original mission outcome, debrief, and player objective observations."""
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'earthsiege-reverse/analysis/missions'
out=bytearray(b'ESMOBJ01')
def write(*values):out.extend(struct.pack('<'+'i'*len(values),*values))
rows=json.loads((source/'objectives.json').read_text())['cases'];write(len(rows))
for r in rows:
 t=r['type']
 if t=='target_grade':write(0,r['grade'],r['dead'],r['removed'],r['result'])
 elif t in ['group_grade','mixed_group_grade']:write(1,len(r['grades']),*r['grades'],*r['disabled'],r['result'])
 elif t=='world_boundary':write(2,r['margin'],*r['position'],r['result'])
 elif t=='safe':write(3,r['distance'],r['targeting'],r['recognized'],r['exclusion_flag'],r['waiting'],r['same_side'],r['result'])
 elif t=='outcome':write(4,r['kind'],r['complete'],r['danger'],['inside','warning','outside'].index(r['boundary']),r['request'],r['result'])
 elif t=='player_status':write(5,r['kind'],r['dead'],r['disabled'],r['result'])
 elif t=='argument_6':write(6,r['grade'],r['danger'],r['result'])
 elif t=='outcome_clock':write(7,['dead','disabled','complete','warning','unfinished'].index(r['state']),r['ratio'],r['frame'],r['result'],r['poll'],r['delay'],r['armed'],r['previous'])
 else:raise ValueError(t)
rows=json.loads((source/'campaign.json').read_text())['results']['cases'];write(len(rows))
for r in rows:write(*r)
rows=json.loads((source/'player-objectives.json').read_text())['cases'];write(len(rows))
for r in rows:
 write(r['leader'],r['kind'],r['selected'],len(r['refs']),*r['refs'],r['cursor'])
 for point in r['points']:write(*point)
 write(*r['position'],*r['target'],r['heading'],r['torso'],r['dead'],r['waiting'])
 write(r['reached'],r['rescued'],r['notified'],r['stage'],r['timer'],r['ratio'])
 write(len(r['events']),*r['events'])
 for key in ['cursor','reached','rescued','notified','stage','timer']:write(r['final'][key])
rows=json.loads((source/'objective-worlds.json').read_text())['cases'];write(len(rows))
for r in rows:
 for actor in r['actors']:write(actor['group'],*actor['position'],actor['damage'],*actor['flags'],actor['selected'],*actor['known'])
 for group in r['groups']:write(group['order'],group['side'],group['waiting'],group['complete'],group['target_group'],group['target'])
 write(r['kind'],r['requested'],*r['safe'],*r['grades'],*r['target_safe'],*r['target_grades'],*r['target_route'],r['status'],*r['results'])
(root/'tests/fixtures/mission_objectives.bin').write_bytes(out)
print(len(out),'bytes of original mission decisions')
