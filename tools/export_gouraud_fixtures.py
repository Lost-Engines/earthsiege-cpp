#!/usr/bin/env python3
"""Import original Gouraud band-emission and complete framebuffer checksums."""
import argparse
import json
from pathlib import Path
import struct
import zlib
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
bands = json.loads((args.analysis / 'models/native-gouraud-bands.json').read_text())['cases']
rasters = json.loads((args.analysis / 'models/native-gouraud-raster.json').read_text())
with (root / 'tests/fixtures/gouraud_raster.bin').open('wb') as out:
    def triangle(row):
        for p in row['points']:out.write(struct.pack('<2i', *p))
        out.write(bytes(row['colours']))
    out.write(b'ESGOURA1')
    out.write(struct.pack('<IIB', len(bands), len(rasters['cases']), rasters['background']))
    for row in bands:
        triangle(row)
        stream = bytearray()
        for band in row['bands']:
            stream.extend(bytes([len(band['points']),band['colour']]))
            for p in band['points']:stream.extend(struct.pack('<2i',*p))
        out.write(struct.pack('<HI',len(row['bands']),zlib.crc32(stream)))
    for row in rasters['cases']:
        triangle(row)
        out.write(struct.pack('<4iI',*row['bounds'],row['crc32']))
print(len(bands),'original band emissions and',len(rasters['cases']),'complete Gouraud rasters imported')
