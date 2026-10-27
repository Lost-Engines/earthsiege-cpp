#!/usr/bin/env python3
"""Import original bullet, rocket and beam world-hit requests."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/projectile-packets.json').read_text())
out=bytearray(b'ESPKTS01')+struct.pack('<II',len(data['flight']),len(data['beams']))
def packet(p):
    global out
    out+=struct.pack('<9hBB3iih16h',*p['transform'],p['reach'],p['radius'],p['armour'],p['shield'],p['splash'],*p['effects'],p['missile_subtype'])
for r in data['flight']:
    out+=struct.pack('<4hB3H3iB3i',r['kind'],r['subtype'],r['charge'],r['ratio'],r['hit'],*r['angles'],*r['position'],r['done'],*r['final_position']);packet(r['packet'])
for r in data['beams']:
    out+=struct.pack('<2h2i3H3i6i',r['subtype'],r['charge'],r['reach'],r['depth'],*r['angles'],*r['position'],*r['beam']['start'],*r['beam']['end']);packet(r['packet'])
(root/'tests/fixtures/projectile_packets.bin').write_bytes(out)
print(len(data['flight']),len(data['beams']),'original flight/beam packets')
