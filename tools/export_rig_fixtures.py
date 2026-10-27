#!/usr/bin/env python3
"""Import original three-channel walking/aiming pose composition."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'models/aim-rigs'
rows=[json.loads(p.read_text())for p in sorted(source.glob('*.json'))if p.name!='catalogue.json']
out=bytearray(b'ESRIGS01')+struct.pack('<H',len(rows));total=0
for model in rows:
 out+=model['model'].encode()+b'\0'+struct.pack('<hhH',model['yaw_sequence'],model['pitch_sequence'],len(model['priorities']))
 out+=struct.pack('<'+'h'*len(model['priorities']),*model['priorities'])+struct.pack('<H',len(model['cases']))
 for row in model['cases']:
  out+=struct.pack('<5h',*[row[k]for k in ['sequence','frame','elapsed','yaw','pitch']])
  for channel in row['channels']:out+=struct.pack('<15h',*channel)
  out+=bytes(row['order'])+struct.pack('<H',len(row['poses']))
  for pose in row['poses']:out+=struct.pack('<3H3h',*pose)
  for matrix in row['matrices']:out+=struct.pack('<9hBB3i',*matrix)
  total+=1
(root/'tests/fixtures/animation_rigs.bin').write_bytes(out)
print(total,'complete original layered poses imported')
