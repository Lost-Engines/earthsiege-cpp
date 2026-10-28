#!/usr/bin/env python3
"""Import original missile-camera page rasters and lifetime transitions."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/missile-displays.json').read_text())
out=bytearray(b'ESMIS001')+struct.pack('<10hI',*data['header'],len(data['cases']))
for r in data['cases']:
    out+=struct.pack('<5iBBIH',*r['bounds'],r['watch'],r['ready'],r['locked'],r['now'],len(r['missiles']))
    for m in r['missiles']:out+=struct.pack('<I3i3H',m['id'],*m['position'],*m['angles'])
    out+=struct.pack('<H',len(r['explosions']))
    for e in r['explosions']:out+=struct.pack('<I3i',e['id'],*e['position'])
    out+=struct.pack('<BiiB3i3HI',r['kind'],r['watched'],r['explosion'],r['flash'],*r['retained'],r['crc32'])
(root/'tests/fixtures/missile_displays.bin').write_bytes(out)
print(len(data['cases']),'missile display frames exported')
