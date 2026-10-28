#!/usr/bin/env python3
"""Pack original flyer/world integration frames."""
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1];rows=json.loads((root.parent/'earthsiege-reverse/analysis/missions/flyer-frames.json').read_text())['cases'];out=bytearray(b'ESFFRM01')
def put(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
def rng(r):put('56HBB',*[v&65535 for v in r[:56]],*r[56:])
put('H',len(rows))
for c in rows:
 put('Hh',c['demo'],c['ratio']);rng(c['initial_random']);put('HH',len(c['steps']),len(c['steps'][0]['flyers']))
 for s in c['steps']:
  for a in s['flyers']:put('3i3H3h3hhhBHiB',*a['position'],*a['angles'],*a['velocity'],*a['rates'],a['strength'],a['timer'],a['obstacle'],a['controller'],a['clock'],a['detectable'])
  rng(s['random'])
(root/'tests/fixtures/mission_flyer_frames.bin').write_bytes(out)
print(sum(len(c['steps'])for c in rows),'world frames exported')
