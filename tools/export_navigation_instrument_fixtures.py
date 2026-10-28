#!/usr/bin/env python3
"""Import sequential original heading/torso/elevation raster captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/navigation-instruments.json').read_text())['cases']
out=bytearray(b'ESHUDN03')+struct.pack('<I',len(rows))
for r in rows:
 out+=r['chassis'].encode()+b'\0'+struct.pack('<3h2B2hI',r['heading'],r['torso'],r['pitch'],r['disabled'],r['tracking'],*r['axis_values'],r['crc32'])
(root/'tests/fixtures/navigation_instruments.bin').write_bytes(out)
print(len(rows),'navigation frames exported')
