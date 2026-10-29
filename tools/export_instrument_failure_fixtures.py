#!/usr/bin/env python3
"""Export independently checked original failure timer and final UI-RNG states."""
from pathlib import Path
import json,struct,sys
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/instrument-failure.json').read_text())
out=bytearray(b'ESIFAL01')+struct.pack('<257B',*data['damage_bands'])+struct.pack('<I',len(data['traces']))
for trace in data['traces']:
 out+=struct.pack('<HHI',trace['seed'],trace['severity'],len(trace['steps']))
 for frame in trace['steps']:
  assert frame['sounds']in[[],[28]]
  out+=struct.pack('<IHII3B',frame['clock'],frame['duration'],frame['window'],frame['next_flip'],frame['failed'],frame['dirty'],bool(frame['sounds']))
 out+=struct.pack('<56h2B',*trace['final_random'])
(root/'tests/fixtures/instrument_failure.bin').write_bytes(out)
print(data['updates'],'original failure transitions exported')
