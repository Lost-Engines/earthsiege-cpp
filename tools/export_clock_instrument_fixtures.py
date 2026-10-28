#!/usr/bin/env python3
"""Import original cockpit clock paints, scheduling and rollover captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/clock-instruments.json').read_text())['cases']
out=bytearray(b'ESCLOCK1')+struct.pack('<I',len(rows))
for r in rows:
 out+=r['chassis'].encode()+b'\0'+struct.pack('<IB',r['now'],r['full'])+r['text'].encode()+struct.pack('<BII',r['colon'],r['deadline'],r['crc32'])
(root/'tests/fixtures/clock_instruments.bin').write_bytes(out)
print(len(rows),'clock frames exported')
