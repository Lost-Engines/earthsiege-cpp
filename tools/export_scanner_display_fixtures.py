#!/usr/bin/env python3
"""Import unchanged original scanner renders with their complete input state."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'cockpits/scanner-displays.json').read_text())['cases']
out=bytearray(b'ESSCAN01')+struct.pack('<I',len(rows))
for r in rows:
    out+=struct.pack('<6iHIiBH',*r['bounds'],*r['position'],r['heading'],r['now'],r['range'],r['radar_active'],len(r['contacts']))
    for c in r['contacts']:
        flags=sum((1<<i)for i,k in enumerate(['active','visible','last_position_unavailable','selected'])if c[k])
        out+=struct.pack('<hB4iB',c['kind'],c['side'],*c['position'],*c['last_position'],flags)
    out+=struct.pack('<I',r['crc32'])
(root/'tests/fixtures/scanner_displays.bin').write_bytes(out)
print(len(rows),'scanner display frames exported')
