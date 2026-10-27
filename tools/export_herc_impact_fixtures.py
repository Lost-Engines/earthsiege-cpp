#!/usr/bin/env python3
"""Retain original HERC interception state and the resulting body-damage requests."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/herc-impacts.json').read_text())
out=bytearray(b'ESHERC01')+struct.pack('<H',len(data['checks']))
def state(s):return struct.pack('<4hBBhi',*s['shields'],s['flags']['detectable'],s['flags']['shield_warning'],s['ecm_timer'],s['damage_received'])
for field in data['checks']:
    out+=field['name'].encode()+b'\0'+struct.pack('<H'+'h'*len(field['equipment']),len(field['equipment']),*field['equipment'])
    out+=struct.pack('<2hH',field['shield_height'],field['shield_radius'],len(field['cases']))
    for r in field['cases']:
        out+=struct.pack('<3iHi5hBBhB',*r['position'],r['heading'],r['reach'],r['radius'],r['armour_damage'],r['shield_damage'],r['splash'],r['signature'],r['player'],r['attacker_side'],r['difficulty'],r['override'])
        out+=state(r['initial'])+state(r['final'])+struct.pack('<i',r['result'])
        damage=next((e for e in r['events']if e[0]=='damage'),None);radial=next((e for e in r['events']if e[0]=='radial'),None)
        out+=struct.pack('<B',damage is not None)
        if damage:out+=struct.pack('<3h',damage[2],damage[1],radial[-1]if radial else 0)
        position=radial[2:5]if radial else next((e[2:]for e in r['events']if e[0]=='explosion'),None)if not damage else None
        out+=struct.pack('<BB',position is not None,sum(e==['speech',8]for e in r['events']))
        if position is not None:out+=struct.pack('<3i',*position)
(root/'tests/fixtures/herc_impacts.bin').write_bytes(out)
print(sum(len(r['cases'])for r in data['checks']),'original HERC interception references')
