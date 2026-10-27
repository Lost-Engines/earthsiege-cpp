#!/usr/bin/env python3
"""Import original muzzle positions, spawn requests and post-discharge state."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'definitions/weapon-firing.json').read_text())['cases']
out=bytearray(b'ESFIRE01')+struct.pack('<I',len(rows))
for row in rows:
    out+=row['name'].encode()+b'\0'
    for key in ['equipment','values']:out+=struct.pack('<H'+'h'*len(row[key]),len(row[key]),*row[key])
    out+=struct.pack('<3hB3H3i2h',row['weapon'],row['slot'],row['amount'],row['free'],*row['angles'],*row['position'],*row['aim'])
    out+=struct.pack('<7h',*(row['state'][k]for k in ['remaining','flags','refreshed','firing','available','holding','amount']))
    out+=struct.pack('<H',len(row['shots']))
    for shot in row['shots']:
        out+=struct.pack('<Bhih3i3H9hBB3i',['bullet','rocket','scaled','beam'].index(shot['kind']),shot['charge'],shot['range'],shot['subtype'],*shot['position'],*shot['angles'],*shot['transform'])
(root/'tests/fixtures/weapon_firing.bin').write_bytes(out)
print(len(rows),'original discharges')
