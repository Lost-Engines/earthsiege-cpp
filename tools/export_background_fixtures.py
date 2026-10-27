#!/usr/bin/env python3
"""Import original horizon and gradient drawing commands."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'worlds/native-background-bands.json').read_text())
out=bytearray(b'ESBANDS2')+struct.pack('<H',len(rows))
for row in rows:
 out+=struct.pack('<HH',row['world'],len(row['cases']))
 for case in row['cases']:
  out+=struct.pack('<3H4iBBI',case['pitch'],case['roll'],case['shift'],*case['centre'],*case['offset'],bool(case.get('fault')),'crc32'in case,case.get('crc32',0))
  if case.get('fault'):continue
  out+=struct.pack('<6iH',*case['horizon'],len(case['commands']))
  for command in case['commands']:
   out+=struct.pack('<BIH',{'rect':0,'sky':1,'ground':2}[command['kind']],command['colour'],len(command['points']))
   for point in command['points']:out+=struct.pack('<2i',*point)
(root/'tests/fixtures/background_bands.bin').write_bytes(out)
print(sum(len(r['cases'])for r in rows),'original background cases exported')
