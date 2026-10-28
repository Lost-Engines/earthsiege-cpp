#!/usr/bin/env python3
"""Import original NAV MAP indexed render checksums and world/camera inputs."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
worlds=json.loads((Path(sys.argv[1])/'cockpits/navigation-displays.json').read_text())['worlds']
out=bytearray(b'ESNAV001')+struct.pack('<H',len(worlds))
for w in worlds:
    out+=struct.pack('<10hH',*w['header'],len(w['cases']))
    for r in w['cases']:
        out+=struct.pack('<7i3HI',*r['bounds'],*r['position'],*r['angles'],r['crc32'])
(root/'tests/fixtures/navigation_displays.bin').write_bytes(out)
print(sum(len(w['cases'])for w in worlds),'navigation display frames exported')
