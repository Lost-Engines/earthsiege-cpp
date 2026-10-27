#!/usr/bin/env python3
"""Retain original ordered world-hit callbacks and ground-impact results."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/hit-dispatch.json').read_text())
out=bytearray(b'ESHITD01')+struct.pack('<II',len(data['checks']),len(data['ground']))
for row in data['checks']:
    out+=struct.pack('<ii',row['reach'],row['result'])
    for a in row['actors']:
        out+=struct.pack('<8Bi',*(a[k]for k in ['side','same','waiting','dead','disabled','inactive','quiet','action']),a['depth'])
    out+=struct.pack('<H',len(row['events']))
    for event in row['events']:
        out+=struct.pack('<B',{'query':0,'trigger':1,'react':2,'friendly-warning':3}[event[0]])
        out+=struct.pack('<'+'i'*(len(event)-1),*event[1:])
for row in data['ground']:
    out+=struct.pack('<2H3iB',row['pitch'],row['yaw'],row['height'],row['reach'],row['result'],len(row['effects']))
    for position,effect in row['effects']:out+=struct.pack('<3ih',*position,effect)
(root/'tests/fixtures/hit_dispatch.bin').write_bytes(out)
print(len(data['checks']),'actor dispatch references and',len(data['ground']),'ground impacts')
