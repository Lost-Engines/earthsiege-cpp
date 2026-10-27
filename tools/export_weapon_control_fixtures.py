#!/usr/bin/env python3
"""Import original weapon updates, recharge schedules and shield allocations."""
import json
import struct
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1]) / 'definitions'
kinds = ['range', 'ready', 'charge', 'elf', 'charge_setting', 'common', 'setting_preset', 'shield', 'balance', 'capacitor_schedule', 'reactor']
rows = json.loads((analysis / 'weapon-controls.json').read_text())['cases'] + json.loads((analysis / 'power.json').read_text())['cases']
for r in json.loads((analysis / 'reactor.json').read_text())['cases']:
    rows.append(['reactor', r['reserve'], r['ratio'], r['light'], r['heavy'], *r['shields'], r['returned'], r['received'], r['result'], *r['final_shields']])
out = bytearray(b'ESPOWER1') + struct.pack('<I', len(rows))
for kind, *values in rows:
    if kind == 'capacitor_schedule':
        selected, budget, cheat, pointers, initial, order, result, final = values
        values = [selected, budget, cheat, len(pointers)]
        for present, before, after in zip(pointers, initial, final):
            values += [bool(present), before['target'], before['charge'], before['fast'], before['disabled'], after['charge'], after['fast']]
        values += [result]
    out += struct.pack('<BH' + 'i' * len(values), kinds.index(kind), len(values), *values)
(root / 'tests/fixtures/weapon_controls.bin').write_bytes(out)
print(len(rows), 'original weapon and shield cases')
