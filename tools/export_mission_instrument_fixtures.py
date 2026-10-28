#!/usr/bin/env python3
"""Import original mission objects and next-route points consumed by the HUD."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
worlds=json.loads((Path(sys.argv[1])/'cockpits/mission-instruments.json').read_text())['worlds']
out=bytearray(b'ESMHUD01')+struct.pack('<I',len(worlds))
for w in worlds:
 out+=struct.pack('<II',w['demo'],len(w['actors']))
 for a in w['actors']:out+=struct.pack('<3ihB',*a['position'],a['radius'],a['side'])
 out+=struct.pack('<B',w['waypoint'] is not None)
 if w['waypoint']:out+=struct.pack('<h3i',w['waypoint']['number'],*w['waypoint']['position'])
(root/'tests/fixtures/mission_instruments.bin').write_bytes(out)
print(len(worlds),'original mission HUD starts exported')
