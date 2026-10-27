#!/usr/bin/env python3
"""Export complete original mixed effect-world frames and their shared random state."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/effect-world.json').read_text())['cases']
out=bytearray(b'ESFXWR01')+struct.pack('<H',len(rows))
def words(v):return struct.pack('<H'+'H'*len(v),len(v),*v)
def rng(v):return struct.pack('<56H2B',*[x&65535 for x in v[:56]],*v[56:])
def location(v):return struct.pack('<3i3H',*v['position'],*v['angles'])
for row in rows:
 out+=struct.pack('<h',row['ratio'])+rng(row['initial_random'])+struct.pack('<H',len(row['steps']))
 for frame in row['steps']:
  out+=struct.pack('<H',len(frame['requests']))
  for request in frame['requests']:out+=struct.pack('<5i',*request)
  for category in ['order','retired']:
   for key in ['explosions','debris']:out+=words(frame[category][key])
  out+=struct.pack('<H',len(frame['explosions']))
  for v in frame['explosions']:
   out+=struct.pack('<Hh',v['slot'],v['definition'])+location(v)+struct.pack('<h',v['timer'])+words(v['cells'])+struct.pack('<hh',v['secondary']if v['secondary']is not None else -1,v['light']if v['light']is not None else -1)
  out+=struct.pack('<H',len(frame['debris']))
  for v in frame['debris']:out+=struct.pack('<Hh',v['slot'],v['model'])+location(v)+struct.pack('<3h',*v['velocity'])
  out+=rng(frame['random'])
(root/'tests/fixtures/effect_world.bin').write_bytes(out)
print(sum(len(r['steps'])for r in rows),'mixed effect-world frames')
