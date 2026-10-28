#!/usr/bin/env python3
"""Import original reactor instrument raster comparisons."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])/'cockpits/reactor-gauges.json'
rows=json.loads(source.read_text())['cases']
out=bytearray(b'ESREACT1')+struct.pack('<I',len(rows))
for row in rows:out+=row['chassis'].encode()+b'\0'+struct.pack('<iI',row['value'],row['crc32'])
(root/'tests/fixtures/reactor_gauges.bin').write_bytes(out)
print(len(rows),'original reactor instrument frames exported')
