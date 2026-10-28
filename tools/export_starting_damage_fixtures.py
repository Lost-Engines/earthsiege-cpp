#!/usr/bin/env python3
"""Export original starting damage and saved-condition restoration captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/starting-damage.json').read_text())
out=bytearray(b'ESSTART1')+struct.pack('<H',len(data['checks']))
flags=['dead','disabled','movement_light','movement_heavy','reactor_light','reactor_heavy','salvage_excluded','collapsed','detectable']
def shorts(v):return struct.pack('<H'+'h'*len(v),len(v),*v)
def state(s):
 return b''.join(shorts(s[k])for k in ['internal','armour','present','cells','weapons'])+struct.pack('<9B56H2B',*[s['flags'][k]for k in flags],*[v&65535 for v in s['random'][:56]],*s['random'][56:])
for index,chassis in enumerate(data['checks']):
 out+=chassis['name'].encode()+b'\0'+shorts(chassis['equipment'])+shorts(chassis['values'])
 out+=struct.pack('<H',len(chassis['cases']))
 for row in chassis['cases']:
  out+=struct.pack('<h',row['condition'])+state(row['initial'])+state(row['final'])+struct.pack('<H',len(row['events']))
  for event in row['events']:out+=struct.pack('<B'+'i'*len(event),len(event),*event)
 # The shell only supports the first eight playable chassis. In particular,
 # the Spider's 12-value capacity table is too short for the roster consumer.
 restores=chassis['restores'] if index<8 else []
 out+=struct.pack('<H',len(restores))
 for row in restores:
  out+=b''.join(struct.pack('<'+'h'*len(v),*v)for v in row['conditions'])+state(row['initial'])+state(row['final'])
(root/'tests/fixtures/starting_damage.bin').write_bytes(out)
print(len(data['checks']),'chassis exported')
