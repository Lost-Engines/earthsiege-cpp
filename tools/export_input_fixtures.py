#!/usr/bin/env python3
"""Import original held-key and frame-time traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
analysis=Path(sys.argv[1])
keyboard=json.loads((analysis/'interface/native-keyboard.json').read_text())
clock=json.loads((analysis/'tapes/clock.json').read_text())
out=bytearray(b'ESINPUT1')
def state(value):return bytes(x for group in value for x in group)
def sample(value):return struct.pack('<4i2B',*value[:6])
out+=struct.pack('<I',len(keyboard['accumulations']))
for row in keyboard['accumulations']:
 out+=state(row['before'])+struct.pack('<2B',row['mode'],row['alt'])+sample(row['output'])+state(row['after'])
out+=struct.pack('<I',len(keyboard['transitions']))
for row in keyboard['transitions']:
 out+=struct.pack('<3B',row['key'],row['up'],row['alt'])+state(row['after_event'])+bytes([row['sample']is not None])
 if row['sample']:
  s=row['sample'];out+=bytes([s['mode']])+sample(s['output'])+state(s['after'])
out+=struct.pack('<H',len(clock['ratio_cases']))
for row in clock['ratio_cases']:out+=struct.pack('<3h',row['elapsed'],row['nominal'],row['ratio'])
out+=struct.pack('<H',len(clock['scale_cases']))
for row in clock['scale_cases']:out+=struct.pack('<hHh',row['base'],row['ratio'],row['result'])
(root/'tests/fixtures/input.bin').write_bytes(out)
print(len(keyboard['accumulations']),'input combinations,',len(keyboard['transitions']),'events,',len(clock['ratio_cases'])+len(clock['scale_cases']),'clock cases')
