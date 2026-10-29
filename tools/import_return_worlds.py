#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
rows=json.loads(Path(sys.argv[1]).read_text())['cases'];out=bytearray(b'ESRETN01'+struct.pack('<I',len(rows)))
for c in rows:
 data=bytes.fromhex(c['result']);out+=c['demo'].upper().encode()+b'\0'+struct.pack('<56H2B',*(v&65535 for v in c['initial_random'][:56]),*c['initial_random'][56:])+struct.pack('<100hI',*c['counters'],len(data))+data
(Path(__file__).resolve().parents[1]/'tests/fixtures/return_worlds.bin').write_bytes(out)
