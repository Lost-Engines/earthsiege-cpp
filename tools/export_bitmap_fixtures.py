#!/usr/bin/env python3
"""Import native transparent bitmap scaling references."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'models/native-bitmap-raster.json').read_text())
out=bytearray(b'ESBITMAP')+struct.pack('<IHHB',len(data['cases']),data['width'],data['height'],data['background'])+bytes(data['pixels'])
for row in data['cases']:out+=struct.pack('<8iI',*row['size'],*row['position'],*row['bounds'],row['crc32'])
(root/'tests/fixtures/bitmap_raster.bin').write_bytes(out)
print(len(data['cases']),'native bitmap scalings imported')
