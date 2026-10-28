#!/usr/bin/env python3
"""Export original world perception passes, contact state, events and RNG."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];d=json.loads((Path(sys.argv[1])/'missions/perception-world.json').read_text());out=bytearray(b'ESPSCAN1')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def state(rows):
 for r in rows:pack('16B2h3B',*r['known'],*r['visible'],r['visibility'],r['refresh'],r['seen'],r['alerted'],r['busy'])
pack('H',len(d['cases']))
for r in d['cases']:
 for a in r['actors']:pack('3iHh8BhhB',*a['position'],a['heading'],a['offset'],a['omni'],a['side'],a['waiting'],a['dead'],a['disabled'],a['player'],a['detectable'],a['trigger'],a['counter'],a['operation'],a['latched'])
 state(r['initial']);pack('56h2B',*r['random']);pack('i',r['range'])
 for row in r['visibility']:pack('8B',*row)
 pack('H',len(r['steps']))
 for s in r['steps']:
  pack('h',s['ratio']);state(s['state']);pack('56h2B',*s['random']);pack('8B8h',*s['latched'],*s['counters']);pack('H',len(s['events']))
  for event in s['events']:pack('3i',*event)
(root/'tests/fixtures/perception_world.bin').write_bytes(out)
print(sum(len(r['steps'])for r in d['cases']),'world perception passes exported')
