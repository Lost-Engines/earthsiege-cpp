#!/usr/bin/env python3
"""Import complete original component-damage states and recorded choice draws."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
analysis=Path(sys.argv[1])/'models/damage'
report=json.loads((analysis/'catalogue.json').read_text())
out=bytearray(b'ESDMG001')
def shorts(values):return struct.pack('<H'+'h'*len(values),len(values),*values)
def state(s):
 result=b''.join(shorts(s[key]) for key in ['internal','armour','present','cells'])
 return result+struct.pack('<6B',*[s['flags'][key] for key in ['dead','disabled','movement_light','movement_heavy','component5_light','component5_heavy']])
out+=struct.pack('<H',len(report['accumulator_cases']))
for r in report['accumulator_cases']:out+=struct.pack('<5hB',r['amount'],r['capacity'],r['current'],*r['result'])
out+=struct.pack('<H',len(report['models']))
for entry in report['models']:
 r=json.loads((analysis/(entry['model']+'.json')).read_text())
 definition=shorts(r['definition']['hit_points'])+struct.pack('<H',len(r['definition']['pieces']))
 for piece in r['definition']['pieces']:
  definition+=struct.pack('<hbbbBH',piece['armour'],piece['debris'],piece['sequence'],piece['parent'],piece['flags'],len(piece['dependents']))
  for pair in piece['dependents']:definition+=struct.pack('<2h',*pair)
 out+=r['model'].encode()+b'\0'+struct.pack('<hI',r['legs'],len(definition))+definition
 out+=shorts(r['equipment_slots'])+state(r['initial'])+struct.pack('<H',len(r['cases']))
 for case in r['cases']:
  out+=struct.pack('<H',len(case['hits']))
  for hit in case['hits']:
   out+=struct.pack('<2hH',hit['part'],hit['amount'],len(hit['component_draws']))
   for draw in hit['component_draws']:out+=struct.pack('<2h',draw['bound'],draw['value'])
   out+=shorts(hit['detached'])+shorts([v if v is not None else -1 for v in hit['weapons_disabled']])+state(hit['result'])
(root/'tests/fixtures/damage.bin').write_bytes(out)
print(sum(r['updates']for r in report['models']),'damage updates and',len(report['accumulator_cases']),'accumulator cases')
