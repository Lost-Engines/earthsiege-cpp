#!/usr/bin/env python3
"""Import original speed LED and throttle slider rasters."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/throttle-gauges.json').read_text())['cases']
out=bytearray(b'ESTHROT1')+struct.pack('<I',len(rows))
for row in rows:out+=row['chassis'].encode()+b'\0'+struct.pack('<6iI',row['speed'],row['request'],*row['thumb'],row['crc32'])
(root/'tests/fixtures/throttle_gauges.bin').write_bytes(out)
print(len(rows),'original throttle frames exported')
