#!/usr/bin/env python3
"""Package unmodified native preference transaction outputs."""
import json,struct,sys
from pathlib import Path
rows=json.loads(Path(sys.argv[1]).read_text())['cases']
out=bytearray(b'ESPREF01'+struct.pack('<I',len(rows)))
for c in rows:
 out.extend(struct.pack('<BIBBBB',c['initial_file'],c['step'],c['operation'],c['index'],c['value'],c['live']))
 out.extend(bytes([len(c['selected']),*c['selected']]))
 for name in ['current','saved','previous']:out.extend(bytes(c[name]))
 out.extend(bytes([len(c['callbacks'])]));out.extend(bytes(v for pair in c['callbacks'] for v in pair))
 out.extend(bytes([len(c['persisted']),*c['persisted']]))
(Path(__file__).resolve().parents[1]/'tests/fixtures/preferences.bin').write_bytes(out)
