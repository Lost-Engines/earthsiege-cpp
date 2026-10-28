#!/usr/bin/env python3
"""Import original weapon/cockpit exchange captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/weapon-display-state.json').read_text())
out=bytearray(b'ESWPND01')+struct.pack('<I',len(data['cases']))
for r in data['cases']:
 out+=struct.pack('<B5B7i6i3i',['energy','ammunition','ecm'].index(r['kind']),*r['flags'],r['setting'],r['charge'],r['requested'],r['rounds'],r['displayed'],r['ratio'],r['enabled'],*r['result'],r['weapon_setting'],r['weapon_displayed'],r['ecm_pulse'])
(root/'tests/fixtures/weapon_display.bin').write_bytes(out)
print(len(data['cases']),'weapon display exchanges exported')
