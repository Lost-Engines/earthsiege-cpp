#!/usr/bin/env python3
"""Import original mounted-weapon trees, including all vertices and node bindings."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=Path(sys.argv[1])
rows=json.loads((source/'models/native-weapon-geometry.json').read_text());assemblies=json.loads((source/'models/assemblies.js').read_text().split('=',1)[1].rstrip(';\n'))['models']
roster=next(r for r in json.loads((source/'tables/tables.json').read_text())if r['name']=='shell0/CAR/SM_INIT.DAT')['hercs']
out=bytearray(b'ESWPN001')+struct.pack('<H',len(assemblies));nodes=points=0
for name,model in assemblies.items():
 equipment=[v[0]for v in next(r for r in roster if r['slot']==model['slot'])['weapons']]
 items=[r for r in rows if r['name']==name]
 out+=name.encode()+b'\0'+struct.pack('<H'+'h'*len(equipment)+'H',len(equipment),*equipment,len(items))
 for item in items:
  out+=struct.pack('<2hH',item['part'],item['weapon'],len(item['tree']))
  for node in item['tree']:
   path=node['path'];out+=struct.pack('<H'+'H'*len(path)+'6h',len(path),*path,*node['header'])
   out+=bytes(['points'in node])
   if 'points'in node:
    out+=struct.pack('<H',len(node['points']))
    for point in node['points']:out+=struct.pack('<3h',*point);points+=1
   out+=bytes(['bsp_transforms'in node])
   if 'bsp_transforms'in node:out+=struct.pack('<H'+'h'*len(node['bsp_transforms']),len(node['bsp_transforms']),*node['bsp_transforms'])
   nodes+=1
(root/'tests/fixtures/weapon_assembly.bin').write_bytes(out)
print(len(rows),'mounted original weapons,',nodes,'nodes,',points,'complete mesh points')
