#!/usr/bin/env python3
"""Import combat records already compared with original resource readers."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'definitions/definitions.json').read_text())
records=[]
def component(c):
 return [c[k]for k in ['armour','debris','sequence','parent','flags']]+[len(c['dependents'])]+[v for pair in c['dependents']for v in pair]
def collision(c):return [c['index'],len(c['spheres'])]+[v for sphere in c['spheres']for v in sphere]
for row in rows:
 if row['kind']=='Damage':
  values=[len(row['hit_points']),*row['hit_points'],len(row['pieces'])]
  for item in row['pieces']:values+=component(item)
  records.append((0,'dmg/'+row['name']+'.DMG',values))
 elif row['kind']=='Collision':
  values=[len(row['parts'])]
  for item in row['parts']:
   values += [item['part'],len(item['states'])]
   for state in item['states']:values+=collision(state)
  records.append((1,'col/'+row['name']+'.COL',values))
weapons=next(r for r in rows if r['kind']=='Weapons')['weapons'];projectiles=next(r for r in rows if r['kind']=='Projectiles')['projectiles'];values=[len(weapons)]
for weapon in weapons:
 values+=component(weapon['piece'])+collision(weapon['collision']);d=weapon['decoded']
 values+=list(d['model_indices'])+[d[k]for k in ['internal_maximum','minimum_range','range','shot_value_penalty']]+list(d['energy_thresholds'])+[d[k]for k in ['magazine','barrels','projectile']]+list(d['muzzle_offset'])+[d['side_muzzle_offset'][0],0,d['side_muzzle_offset'][2]]+[d[k]for k in ['refire_delay','combat_rating','damage_icon']]
values+=[len(projectiles)]
for p in projectiles:values += [p[k]for k in ['kind','subtype','shield_damage','armour_damage','splash_q10','speed']]+p['shield_effects']+p['ground_effects']+p['armour_effects']
records.append((2,'dat/WEAPONS.DAT',values))
out=bytearray(b'ESCOMBT1')+struct.pack('<H',len(records))
for kind,name,values in records:out+=bytes([kind])+name.encode()+b'\0'+struct.pack('<I'+'i'*len(values),len(values),*values)
(root/'tests/fixtures/combat.bin').write_bytes(out)
print(len(records),'original-reader-verified combat resources')
