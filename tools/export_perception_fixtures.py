#!/usr/bin/env python3
"""Export original recognition, contact sharing and visibility cache decisions."""
import json
from pathlib import Path
import struct
import sys
root = Path(__file__).resolve().parents[1]
rows = json.loads((Path(sys.argv[1]) / 'missions/perception.json').read_text())['cases']
with (root / 'tests/fixtures/perception.bin').open('wb') as out:
    def pack(fmt, *v): out.write(struct.pack('<' + fmt, *v))
    out.write(b'ESPER001'); pack('I', len(rows))
    for kind, *r in rows:
        if kind == 'recognition':
            pack('BBB i B', 0, *r)
        elif kind == 'share':
            source, target, sides, waiting, positions, busy, latched, initial, operations, result = r
            pack('BBBB', 1, len(sides), source, target)
            for values in zip(sides, waiting, positions, busy, latched, initial, operations, result[0], result[2], result[3]):
                side, waits, pos, busy, latched, initial, operation, known, final_latch, counter = values
                pack('BB3iBBhhBBh', side, waits, *pos, busy, latched, initial, operation, known, final_latch, counter)
            pack('B', result[1])
        elif kind == 'visibility_cache':
            pack('B6h', 2, *r)
        else:
            raise ValueError(kind)
print(len(rows), 'original perception decisions imported')
