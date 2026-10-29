#!/usr/bin/env python3
"""Join retained original result bytes to their original captured damage inputs."""
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);out=bytearray(b'ESRES001')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def words(v):pack('H',len(v));pack('h'*len(v),*v)
models=json.loads((source/'missions/results/catalogue.json').read_text())['models'];pack('H',len(models))
for entry in models:
 name=entry['model'];d=json.loads((source/'models/damage'/f'{name}.json').read_text());r=json.loads((source/'missions/results'/f'{name}.json').read_text());states=[d['initial']]+[h['result']for c in d['cases']for h in c['hits']];assert len(states)==len(r['cases'])
 out.extend(name.encode()+b'\0');pack('hI',r['hull_value'],len(states))
 for s,c in zip(states,r['cases']):
  for k in ['internal','armour','present']:words(s[k])
  pack('36h',*c['return_words']);pack('iH',c['recoverable_value'],len(c['salvage']))
  for pair in c['salvage']:pack('2h',*pair)
(Path(__file__).resolve().parents[1]/'tests/fixtures/results.bin').write_bytes(out)
