#!/usr/bin/env python3
"""Import original instance, skeletal pose and channel reference captures."""
import argparse
import json
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
models = args.analysis / 'models'
traces = json.loads((models / 'native-channels.json').read_text())
records = []
for row in json.loads((models / 'native-instances.json').read_text())['cases']:
    records.append((row['volume'] + '/' + row['name'], row['root'], [-1, 0, 0, 0, 0], row['poses'], row['transforms'], row['cells']))
for row in json.loads((models / 'native-channel-poses.json').read_text())['cases']:
    trace = traces['cases'][row['trace']]
    values = trace['states'][row['step']][0]
    elapsed, duration = values[12:14]
    fraction = int((int(duration / 2) + elapsed * 1024) / duration) if duration else 0
    records.append((trace['name'], trace['root'], values[:4] + [fraction], row['poses'], row['matrices'], []))
with (root / 'tests/fixtures/model_poses.bin').open('wb') as out:
    out.write(b'ESPOSE01' + struct.pack('<I', len(records)))
    for name, index, state, poses, matrices, cells in records:
        out.write(name.encode() + b'\0')
        out.write(struct.pack('<H5hH', index, *state, len(poses)))
        for pose in poses:
            out.write(struct.pack('<3H3h', *pose))
        out.write(struct.pack('<H', len(matrices)))
        for matrix in matrices:
            out.write(struct.pack('<9hBB3i', *matrix))
        out.write(struct.pack('<H' + 'H' * len(cells), len(cells), *cells))
print(len(records), 'original default and blended model poses imported')
