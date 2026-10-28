#!/usr/bin/env python3
"""Import original multifunction-display button paints and geometry."""
import json, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=json.loads((Path(sys.argv[1])/'cockpits/mfd-controls.json').read_text())
rows=source['cases'];out=bytearray(b'ESMFDUI1')+struct.pack('<I',len(rows))
for r in rows:
    out+=r['chassis'].encode()+b'\0'+struct.pack('<2iB',*r['offset'],r['mode'])+bytes(r['enabled'])+bytes(r['faces'])
    out+=struct.pack('<56iI',*[v for bounds in r['button_bounds']for v in bounds],*r['display'],r['crc32'])
out+=struct.pack('<I',len(source['ranges']))
for r in source['ranges']:out+=struct.pack('<4i',r['before'],r['returned'],r['after'],r['distance'])
(root/'tests/fixtures/mfd_controls.bin').write_bytes(out)
print(len(rows),'MFD button frames exported')
