#!/usr/bin/env python3
"""Import original target bracket projection and pixel captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/target-instruments.json').read_text())['cases']
out=bytearray(b'ESHUDT01')+struct.pack('<I',len(rows))
for r in rows:
 out+=struct.pack('<5i3H4i4B4iI',*r['centre'],*r['camera'],*r['angles'],*r['position'],r['radius'],r['locked'],r['side'],r['present'],r['visible'],*r['bounds'],r['crc32'])
(root/'tests/fixtures/target_instruments.bin').write_bytes(out)
print(len(rows),'target bracket frames exported')
