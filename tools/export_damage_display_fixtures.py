#!/usr/bin/env python3
"""Import original STATUS/TARGET diagram and label paint fixtures."""
import json, struct, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1]) / 'cockpits/damage-displays.json').read_text())['cases']
out = bytearray(b'ESDAMAGE') + struct.pack('<I', len(rows))
for row in rows:
    s = row['state']
    flags = sum((1 << i) for i, name in enumerate(['friendly', 'player', 'dead', 'shield_warning']) if s[name])
    out += row['chassis'].encode() + b'\0' + row['label'].encode() + b'\0'
    out += struct.pack('<4i3hB31hI', *row['bounds'], s['kind'], s['model'], s['total_damage'], flags,
                       *s['armour'], *s['internal'], row['crc32'])
(root / 'tests/fixtures/damage_displays.bin').write_bytes(out)
print(len(rows), 'damage display frames exported')
