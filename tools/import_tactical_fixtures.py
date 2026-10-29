#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);destination=Path(__file__).resolve().parents[1]/'tests/fixtures'
data=json.loads((source/'cockpits/tactical-map.json').read_text())
p=bytearray(b'ESTMAP01')
def pack(fmt,*v):p.extend(struct.pack('<'+fmt,*v))
pack('10h',*data['header']);pack('4i',*data['bounds']);pack('4i',*data['limits']);pack('iI',data['initial_zoom'],len(data['cases']))
for row in data['cases']:
 pack('3iIBH',*row['camera'],row['ticks'],row['failed'],len(row['contacts']))
 for c in row['contacts']:
  pack('3h6BH3i2iB',c['kind'],c['base'],c['size'],c['side'],c['dead'],c['hidden'],c['detected'],c['last_missing'],0,c['heading'],*c['position'],*c['last'],c['selected'])
 pack('I',row['crc32'])
(destination/'tactical_map.bin').write_bytes(p)
