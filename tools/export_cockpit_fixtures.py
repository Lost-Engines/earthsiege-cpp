#!/usr/bin/env python3
"""Import original masked model rasters and cockpit resource checks."""
import json, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])
rows=json.loads((source/'cockpits/masked-models.json').read_text())['cases']
bindings=json.loads((source/'models/texture-bindings.js').read_text().split('=',1)[1].rstrip(';\n'))
lookup={name.lower():data['roots'] for name,data in bindings.items()}
out=bytearray(b'ESMASK01')+struct.pack('<I',len(rows))
for row in rows:
 for field in [row['resource'],lookup[row['resource'].lower()][row['root']] or '', 'simvol0/dpl/WORLD0.DPL',row['mask']]:out+=field.encode()+b'\0'
 out+=struct.pack('<H3H3iI',row['root'],*row['angles'],*row['translation'],row['crc32'])
(root/'tests/fixtures/cockpit_masks.bin').write_bytes(out)
print(len(rows),'masked model fixtures exported')

layouts={r['name']:r for r in json.loads((source/'interface/resources.json').read_text())}
rows=json.loads((source/'cockpits/catalogue.json').read_text())
out=bytearray(b'ESCOCK02')+struct.pack('<I',len(rows))
names=['ROADRUNN','GUNSLING','RAPTOR','TOMAHAWK','PATRIOT','PANTHER','HERCULES','COLOSSUS']
for row in rows:
 name=row['name'];out+=name.encode()+b'\0'+struct.pack('<I',names.index(name))
 out+=struct.pack('<425i',*layouts['simvol0/gau/'+name+'.GAU']['runtime_words'])
 for view in row['views']:
  out+=struct.pack('<8iI',*view['window'],*view['projection_origin'],*view['origin'],view['indexed_crc32'])
  out+=struct.pack('<2i',*view['projection_centre'])
 for i in [*range(32,48),*range(64,80)]:
  red,green,blue=row['palette_overrides'][str(i)]
  out+=struct.pack('<I',0xff000000|(red<<16)|(green<<8)|blue)
(root/'tests/fixtures/cockpit_artwork.bin').write_bytes(out)
print(len(rows),'cockpit layouts, palettes and all four artwork views exported')

if (source/'cockpits/scenery-frames.json').exists():
 data=json.loads((source/'cockpits/scenery-frames.json').read_text());rows=data['cases']
 out=bytearray(b'ESCPWLD1')+struct.pack('<H10hH',1,*data['header'],len(rows))
 for row in rows:
  out+=row['cockpit'].encode()+b'\0'+struct.pack('<B',row['view'])
  out+=struct.pack('<3i3HIH',*row['position'],*row['angles'],row['crc32'],len(row['order']))
  out+=struct.pack('<'+'H'*len(row['order']),*row['order'])
 (root/'tests/fixtures/cockpit_scenery.bin').write_bytes(out)
 print(len(rows),'cockpit scenery fixtures exported')
