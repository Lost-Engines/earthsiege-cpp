#!/usr/bin/env python3
"""Export original base construction and complete VGA frame checks."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'definitions/ground-actors.json').read_text())
out=bytearray(b'ESGBASE1')+struct.pack('<H',len(data['cases']))
for r in data['cases']:
 out+=struct.pack('<BhhhBH',r['side'],r['type'],r['condition'],r['damage'],r['dead'],len(r['cells']))
 out+=struct.pack('<'+'H'*len(r['cells']),*r['cells'])+struct.pack('<H',len(r['frames']))
 for f in r['frames']:out+=struct.pack('<3i3HI',*f['position'],*f['angles'],f['crc32'])
(root/'tests/fixtures/ground_actors.bin').write_bytes(out)
print(len(data['cases']),'ground actor starts exported')
