#!/usr/bin/env python3
"""Import scheduler packets captured from the original HMI, before hardware output."""
from pathlib import Path
import json,struct,sys
root=Path(__file__).resolve().parents[1];analysis=Path(sys.argv[1]);music=analysis/'music';original=analysis.parent/'extracted/cd_1994_10'
checks=json.loads((music/'native-checks.json').read_text());out=bytearray(b'ESHMP001')+struct.pack('<I',len(checks['cases']));packets=0
for row in checks['cases']:
 name=row['name'];path=original/'resources'/name
 if not path.exists():path=original/name
 data=path.read_bytes();events=json.loads((music/'events'/Path(name).with_suffix('.json')).read_text());out+=name.encode()+b'\0'+struct.pack('<I',len(data))+data
 deltas=[e for t in events['tracks']for e in t['events']];out+=struct.pack('<I',len(deltas))
 for e in deltas:out+=struct.pack('<II',e['offset'],e['delta'])
 out+=struct.pack('<B',len(row['fixtures']))
 for fixture in row['fixtures']:
  out+=struct.pack('<B',len(fixture['devices']))+struct.pack('<'+'I'*len(fixture['devices']),*fixture['devices'])+bytes(fixture['routing'])+struct.pack('<B','capture'in fixture)
  if 'capture'not in fixture:continue
  native=json.loads((music/fixture['capture']).read_text());out+=struct.pack('<IBI',native['ticks'],native['still_active'],len(native['packets']))
  for event in native['packets']:
   wire=bytes.fromhex(event['wire']);out+=struct.pack('<IIiH',event['tick'],event['device'],event['source_offset']if event['source_offset']is not None else -1,len(wire))+wire
  packets+=len(native['packets'])
(root/'tests/fixtures/hmp.bin').write_bytes(out);print(len(checks['cases']),'songs,',packets,'original scheduler packets')
