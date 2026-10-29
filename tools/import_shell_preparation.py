#!/usr/bin/env python3
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'earthsiege-reverse/analysis/missions/shell-preparation.json'
cases=json.loads(source.read_text())['cases'];out=bytearray(b'ESSHL001'+struct.pack('<I',len(cases)))
for c in cases:
 out+=struct.pack('<4h',c['seed'],c['mode'],c['category'],c['battle'])
 out+=c['name'].encode()+b'\0'
 for key in ['script','counters','player_prepared','crew','player_final']:
  b=bytes.fromhex(c.get(key,''));out+=struct.pack('<I',len(b))+b
 for key in ['rng_prepared','rng_final']:
  r=c[key];out+=struct.pack('<56h2B',*r['words'],*r['indices'])
(root/'tests/fixtures/shell_preparation.bin').write_bytes(out)
print(len(cases),'original shell preparations')
