#!/usr/bin/env python3
"""Export original HERC weapon choices and complete RNG state."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'definitions/herc-weapon-choice.json').read_text())
out = bytearray(b'ESHCHO01')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
def words(values): pack('H', len(values)); pack('h' * len(values), *values)
pack('H', len(data['cases']))
for row in data['cases']:
    name = row['name'].encode('ascii'); pack('H', len(name)); out.extend(name)
    words(row['equipment']); words(row['values']); pack('H', len(row['trials']))
    for trial in row['trials']:
        pack('H', len(trial['weapons']))
        for w in trial['weapons']:
            pack('B', w is not None)
            if w: pack('B4h3B', w['disabled'], w['remaining'], w.get('setting', -1), w.get('charge', -1), w.get('rounds', -1), w['excluded'], w['channel'], w['ready'])
        pack('Bih3h5B2B2h56h2Bh2Bh', trial['kind'], trial['distance'], trial['facing'], *trial['shields'], trial['tolerance'], *trial['locks'], trial['detectable'], trial['unavailable'], trial['ecm'], trial['delay'], *trial['random'], trial['selected'], trial['final_detectable'], trial['final_unavailable'], trial['final_delay'])
        pack('B', len(trial['events']))
        for event in trial['events']: pack('B', event[1] if event[0] == 'shield' else 2)
        pack('56h2B', *trial['final_random'])
(root / 'tests/fixtures/herc_weapon_choice.bin').write_bytes(out)
print(sum(len(r['trials']) for r in data['cases']), 'original weapon choices exported')
