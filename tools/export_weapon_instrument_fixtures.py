#!/usr/bin/env python3
"""Import original weapon-row constructor/setter/raster captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((Path(sys.argv[1])/'cockpits/weapon-instruments.json').read_text())
out=bytearray(b'ESWPNI01')+struct.pack('<I',len(data['cases']))
for row in data['cases']:
 out+=row['chassis'].encode()+b'\0'+row['name'].encode()+b'\0'
 out+=struct.pack('<BB2i6BI', ['energy','ammunition','ecm'].index(row['kind']),row['slot'],row['value'],row['requested'],row['disabled'],row['selected'],row['included'],row['ready'],row['face'],0,row['crc32'])
 out+=struct.pack('<4i',*row['button'])
(root/'tests/fixtures/weapon_instruments.bin').write_bytes(out)
print(len(data['cases']),'weapon instrument frames exported')
