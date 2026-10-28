#!/usr/bin/env python3
"""Export original HERC tactical movement decisions."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
data = json.loads((Path(sys.argv[1]) / 'missions/herc-tactics.json').read_text())
out = bytearray(b'ESHTAC01')
def pack(fmt, *values): out.extend(struct.pack('<' + fmt, *values))
pack('I', len(data['cases']))
for row in data['cases']:
    pack('B3i3i2H2h', row['mode'], *row['position'], *row['target'], row['heading'], row['target_heading'], row['view'], row['target_view'])
    pack('4hBhih5B', *row['state'], row['ratio'], row['damage'], row['command'], row['actor_flags'], row['target_flags'], row['target_done'], row['base_type'], row['result'])
    pack('5hih2i', *row['relative'])
    pack('4hBhH', *row['next_state'], row['next_command'], len(row['events']))
    for event in row['events']:
        pack('B', event[0] == 'aim')
        pack('h' * (len(event) - 1), *event[1:])
(root / 'tests/fixtures/herc_tactics.bin').write_bytes(out)
print(len(data['cases']), 'original tactical decisions exported')
