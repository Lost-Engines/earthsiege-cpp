#!/usr/bin/env python3
"""Pack original HERC strength, component-selection and retreat captures."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])
data = json.loads((analysis / 'missions/herc-assessment.json').read_text())
starts = {w['demo']: w['initial_random'] for w in json.loads((analysis / 'missions/starting-hercs.json').read_text())['worlds']}
out = bytearray(b'ESHASS01')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
def words(values): pack('H', len(values)); pack('h' * len(values), *values)
pack('H', len(data['worlds']))
for world in data['worlds']:
    pack('H56h2BH', world['demo'], *starts[f"demo{world['demo']}"], len(world['actors']))
    for actor in world['actors']:
        pack('2H', actor['index'], len(actor['cases']))
        for row in actor['cases']:
            for key in ['internal', 'armour', 'disabled', 'ammunition', 'present']: words(row[key])
            pack('hB56h2Bh56h2B', row['rating'], row['collapsed'], *row['random'], row['selected'], *row['final_random'])
pack('H', len(data['retreat']))
for row in data['retreat']:
    pack('7h3B2hihBh2B', row['caution'], *row['damage'], row['flags'], row['kind'], row['base_type'], row['attackers'], row['strength'], row['total_strength'], row['tolerance'], row['result'], row['next_tolerance'], 0 if not row['transition'] else 1 if row['transition'] == ['orders'] else 2, row['strength_reads'])
(root / 'tests/fixtures/herc_assessment.bin').write_bytes(out)
print(sum(len(a['cases']) for w in data['worlds'] for a in w['actors']), 'ratings and component choices;', len(data['retreat']), 'retreat decisions')
