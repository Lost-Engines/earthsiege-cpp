#!/usr/bin/env python3
"""Import original waypoint direction and text captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/waypoint-instruments.json').read_text())['cases']
out=bytearray(b'ESHUDW02')+struct.pack('<I',len(rows))
for r in rows:
 out+=r['chassis'].encode()+b'\0'+struct.pack('<2h6ih2BI',r['heading'],r['tape_heading'],*r['position'],*r['target'],r['cursor'],r['attached'],r['present'],r['crc32'])
(root/'tests/fixtures/waypoint_instruments.bin').write_bytes(out)
print(len(rows),'waypoint frames exported')
