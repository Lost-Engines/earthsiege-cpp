#!/usr/bin/env python3
"""Import combined original scenery/actor impacts and post-hit decisions."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/world-impacts.json').read_text())
out=bytearray(b'ESWORLD1')+struct.pack('<I',len(data['checks']))
for field in data['checks']:
    out+=field['resource'].encode()+b'\0'+struct.pack('<HH',field['root'],len(field['cases']))
    for row in field['cases']:
        ray=row['ray'];out+=struct.pack('<H3iih',ray['heading'],*ray['origin'],ray['reach'],ray['radius'])
        for position,angle in ray['objects']:out+=struct.pack('<3iH',*position,angle)
        out+=struct.pack('<3B',row['owner_side'],row['player'],row['target']is not None)
        if row['target']is not None:out+=struct.pack('<3i',*row['target'])
        for a in row['companions']:out+=struct.pack('<3i2B',*a['position'],a['dead'],a['disabled'])
        for a in row['victims']:out+=struct.pack('<8Bi',*(a[k]for k in ['side','same','waiting','dead','disabled','inactive','quiet','kill']),a['depth'])
        out+=struct.pack('<2iB56h2BH',row['result'],row['final_reach'],row['enemy_hit'],*row['random'],len(row['events']))
        for event in row['events']:
            out+=struct.pack('<B',{'query':0,'react':1,'warning':2,'trigger':3,'effect':4,'obstruction':5}[event[0]])
            out+=struct.pack('<'+'i'*(len(event)-1),*event[1:])
(root/'tests/fixtures/world_impacts.bin').write_bytes(out)
print(data['cases'],'original combined world-impact references')
