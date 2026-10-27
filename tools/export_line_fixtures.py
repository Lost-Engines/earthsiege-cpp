#!/usr/bin/env python3
"""Import original 3Space line clipping and VGA rasters."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'models/native-line-raster.json').read_text())
out=bytearray(b'ESLINES1')+struct.pack('<IB',len(data['cases']),data['colour'])
for row in data['cases']:
 out+=struct.pack('<8iB4iI',*row['points'][0],*row['points'][1],*row['bounds'],row['visible'],*row['clipped'][0],*row['clipped'][1],row['crc32'])
(root/'tests/fixtures/line_raster.bin').write_bytes(out)
print(len(data['cases']),'native lines imported')
