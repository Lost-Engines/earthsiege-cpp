#!/usr/bin/env python3
"""Import original shield instrument and palette captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/shield-instruments.json').read_text())
rows=data['cases'];out=bytearray(b'ESSHLD01')+struct.pack('<I',len(rows))
for row in rows:
 out+=row['chassis'].encode()+b'\0'+struct.pack('<3i2BI',row['front'],row['rear'],row['balance'],*row['buttons'],row['crc32'])
 out+=bytes(v for colour in row['dac']for v in colour)
rows=data['palette_cases'];out+=struct.pack('<I',len(rows))
for row in rows:out+=struct.pack('<2i',row['front'],row['rear'])+bytes(v for colour in row['dac']for v in colour)
(root/'tests/fixtures/shield_instruments.bin').write_bytes(out)
print(len(data['cases']),'shield frames and',len(rows),'palette cases exported')
