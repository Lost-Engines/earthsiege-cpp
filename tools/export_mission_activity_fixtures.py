#!/usr/bin/env python3
"""Pack original mission-activity captures for portable regression checks."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])if len(sys.argv)>1 else root.parent/'earthsiege-reverse/work/research/missions/mission-activity.json'
rows=json.loads(source.read_text())['worlds'];out=bytearray(b'ESMACT01')
def put(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
def seq(values):put('H',len(values));put('h'*len(values),*values)
def state(s):
 put('100hH',*s['counters'],s['stagger']);r=s['random'];put('56HBB',*r['words'],r['first'],r['second']);put('H',len(s['triggers']));put('B'*len(s['triggers']),*s['triggers']);seq(s['waiting'])
 for a in s['actors']:
  put('B'*len(a['known']),*a['known']);put('B'*len(a['visible']),*a['visible']);put('hhBBB',a['visibility'],a['refresh'],a['seen'],a['alerted'],a['busy'])
put('H',len(rows))
for w in rows:
 put('HHH',w['demo'],len(w['actors']),len(w['groups']))
 for a in w['actors']:put('hHHBBhhhhhiB',*[a[k]for k in ['kind','group','member','player','persistent_signature','contact','hit','counter','operation','controller','clock','first']])
 for g in w['groups']:
  put('hhhhB',*[g[k]for k in ['kind','order','formation','cursor','side']]);seq(g['members']);seq(g['targets'])
 state(w['initial']);put('H',len(w['steps']))
 for s in w['steps']:
  put('h',s['ratio'])
  for a in s['inputs']:put('3iHBBB',*a['position'],a['heading'],a['dead'],a['disabled'],a['detectable'])
  put('H',len(s['events']))
  for e in s['events']:put('BHh',0 if e[0]=='dispatch'else 1,e[1],e[2]if len(e)>2 else -1)
  state(s['result'])
(root/'tests/fixtures/mission_activity.bin').write_bytes(out)
print(len(rows),'worlds;',sum(len(w['steps'])for w in rows),'complete activity passes;',len(out),'bytes')
