#!/usr/bin/env python3
"""Import original dynamic light selection and registration traces."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = json.loads((args.analysis / 'models/effect-lighting.json').read_text())
isolated = json.loads((args.analysis / 'models/native-light-manager.json').read_text())
with (root / 'tests/fixtures/effect_lighting.bin').open('wb') as out:
    def pack(fmt, *values):
        out.write(struct.pack('<' + fmt, *values))
    def light(row):
        pack('Bi3i3h2i', row['kind'], row['intensity'], *row.get('position', [0, 0, 0]), *row.get('direction', [0, 0, 0]), row.get('attenuation', 0), row.get('scale', 0))
    out.write(b'ESFXLT01')
    pack('I', len(isolated['cases']))
    for row in isolated['cases']:
        pack('3i3iihB', *row['position'], *row['object'], row['radius'], row['intensity'], row['result'] is not None)
        if row['result'] is not None:
            light(row['result'])
    pack('I', len(data['traces']))
    for trace in data['traces']:
        pack('iiB', trace['scale'], trace['offset'], len(trace['persistent']))
        for row in trace['persistent']:
            light(row)
        pack('I', len(trace['operations']))
        for row in trace['operations']:
            op = ['add', 'remove', 'intensity', 'attenuation', 'select'].index(row['operation'])
            pack('B', op)
            if op == 0:
                pack('3iB', *row['position'], row['slot'])
            elif op == 1:
                pack('B', row['slot'])
            elif op == 2:
                pack('Bh', row['slot'], row['value'])
            elif op == 3:
                pack('Bii', row['slot'], row['scale'], row['offset'])
            else:
                pack('3iiB', *row['position'], row['radius'], len(row['result']))
                for value in row['result']:
                    light(value)
print(len(isolated['cases']), 'isolated selections and', sum(len(t['operations']) for t in data['traces']), 'live operations imported')
