#!/usr/bin/env python3
"""Retain original projectile trajectories and per-step manual/guidance inputs."""
import json
import struct
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
report = json.loads((Path(sys.argv[1]) / 'definitions/projectile-motion.json').read_text())
out = bytearray(b'ESFLY001') + struct.pack('<H', len(report['traces']))
def shorts(values):
    return struct.pack('<H' + 'h' * len(values), len(values), *values)
def state(s):
    return struct.pack('<3i3H3h', *s['position'], *s['angles'], s['speed'], s['age'], s['timer']) + shorts(s['cells'])
for trace in report['traces']:
    out += struct.pack('<hB2h', trace['kind'], trace['subtype'], trace['ratio'], trace['speed_limit'])
    out += trace['mode'].encode() + b'\0' + shorts(trace['motion']) + shorts(trace['frame_counts']) + state(trace['initial'])
    out += struct.pack('<H', len(trace['states']))
    for s in trace['states']:
        c = s['controls']
        out += struct.pack('<3i5B2h', *s['target'], c['player'], c['tracking'], c['detectable'], c['interference'], c['held'], c['yaw'], c['pitch'])
        out += state(s) + struct.pack('<4BB2h', s['done'], s['subtype'], s['target_present'], s['owner_guiding'], *s['manual_after'])
(root / 'tests/fixtures/projectile_flight.bin').write_bytes(out)
print(len(report['traces']), 'trajectories,', sum(len(t['states']) for t in report['traces']), 'original updates')
