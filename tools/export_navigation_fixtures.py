#!/usr/bin/env python3
"""Import original route, formation and obstacle-avoidance decisions."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'missions/navigation.json').read_text())
out=bytearray(b'ESNAV001')
out+=struct.pack('<H',len(data['routes']))
for r in data['routes']:
 out+=struct.pack('<hBhH3iH',r['index'],r['closed'],r['speed'],r['yaw'],*r['origin'],len(r['points']))
 for point in r['points']:out+=struct.pack('<3i',*point)
 out+=struct.pack('<4h',r['next_index'],*r['control'])
out+=struct.pack('<H',len(data['formations']))
for r in data['formations']:
 out+=struct.pack('<3iHh3iH3i3h',*r['leader_position'],r['leader_heading'],r['leader_speed'],*r['position'],r['heading'],*r['offset'],*r['control'])
out+=struct.pack('<H',len(data['avoidance']))
for r in data['avoidance']:
 out+=struct.pack('<3iHhiBhh2h',*r['origin'],r['heading'],r['current'],r['countdown'],r['blocked'],r['bias'],r['flags'],*r['input'])
 for hit in r['scene_hits']:out+=struct.pack('<Bi',hit is not None,hit or 0)
 out+=struct.pack('<H',len(r['actors']))
 for actor in r['actors']:out+=struct.pack('<3ihB',*actor['position'],actor['radius'],actor['waiting'])
 out+=struct.pack('<B',len(r['rays']))
 for a,b in r['rays']:out+=struct.pack('<6i',*a,*b)
 out+=struct.pack('<2h',*r['control'])
(root/'tests/fixtures/navigation.bin').write_bytes(out)
print(len(data['routes'])+len(data['formations'])+len(data['avoidance']),'original navigation decisions')
