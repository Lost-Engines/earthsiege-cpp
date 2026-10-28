#!/usr/bin/env python3
"""Import original FLASH COMM paints, choice flags and emitted command numbers."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/communications-displays.json').read_text())['cases']
out=bytearray(b'ESCOMM01')+struct.pack('<I',len(rows))
for r in rows:out+=struct.pack('<4i4BI',*r['bounds'],r['disabled'],r['alternates'],r['selected'],r['command'],r['crc32'])
(root/'tests/fixtures/communications_displays.bin').write_bytes(out)
print(len(rows),'communications display frames exported')
