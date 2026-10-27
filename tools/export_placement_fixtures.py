#!/usr/bin/env python3
"""Import original actor-constructor output and its complete starting inputs."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=Path(sys.argv[1]);folder=source/'missions/constructed'
paths=sorted(folder.glob('demo*.json'))+sorted(p for p in folder.glob('*/*.json')if not p.name.endswith('.js'))
out=bytearray(b'ESPLACE1')+struct.pack('<I',len(paths));actors=groups=undefined=0
kinds={'hercs':0,'flyers':1,'ground':2}
for path in paths:
 row=json.loads(path.read_text());out+=str(path.relative_to(folder)).encode()+b'\0'
 demo=path.stem if path.parent==folder else 'demo1'
 script=source/'tapes'/demo/'script.dat'if path.parent==folder else source/'missions/compiled'/path.parent.name/(path.stem+'.dat')
 for data in [script.read_bytes(),(source/'tapes'/demo/'player.mec').read_bytes()]:out+=struct.pack('<I',len(data))+data
 for kind in kinds:
  items=row['actors'][kind];out+=struct.pack('<H',len(items))
  for item in items:
   valid='placement_note'not in item
   out+=struct.pack('<HhB',item['source'],item.get('chassis',item.get('type',0)),valid)
   if valid:out+=struct.pack('<3i',*item['position'])
   else:undefined+=1
   out+=struct.pack('<3H2Hh2B',*item['angles'],item['group'],item['member'],item.get('height',0),item.get('player_roster',False),item.get('player',False))
   weapons=item.get('weapons',[]);out+=struct.pack('<H'+'h'*len(weapons),len(weapons),*weapons);actors+=1
 out+=struct.pack('<H',len(row['groups']))
 for group in row['groups']:
  out+=struct.pack('<3hHBhH',group['kind'],group['formation'],group['argument'],group['heading'],group['mode'],group['target_kind'],len(group['members']))
  for member in group['members']:out+=struct.pack('<hH',kinds[member['kind']],member['slot'])
  groups+=1
 out+=struct.pack('<hH',kinds[row['player']['kind']],row['player']['slot'])
(root/'tests/fixtures/actor_placement.bin').write_bytes(out)
print(len(paths),'original missions,',actors,'actors,',groups,'groups,',undefined,'memory-dependent positions')
