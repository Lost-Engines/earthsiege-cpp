#!/usr/bin/env python3
"""Import independently decoded original TTM streams and execution captures."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];analysis=Path(sys.argv[1])
rows=json.loads((analysis/'cockpits/ttm-frames.json').read_text())['cases']
out=bytearray(b'ESTTM001')+struct.pack('<I',len(rows))
for r in rows:
 out+=r['name'].encode()+b'\0'+struct.pack('<hhBiBhhhII',r['elapsed'],r['label'],r['page'],r['mode'],r['latch'],r['remaining'],r['period'],r['jump'],r['random'],r['crc32'])
(root/'tests/fixtures/presentation_frames.bin').write_bytes(out)
programs=sorted((analysis/'presentations/scripts').rglob('*.json'))
out=bytearray(b'ESTTMP01')+struct.pack('<I',len(programs))
for p in programs:
 j=json.loads(p.read_text());raw=bytearray()
 for i in j['instructions']:
  raw+=struct.pack('<H',i['code'])
  if i['code']&15<=7:
   for a in i['args']:raw+=struct.pack('<h',a)
  elif 'inline_hex'in i:raw+=struct.pack('<H',i['args'][0])+bytes.fromhex(i['inline_hex'])
  else:
   s=i['text'].encode('cp437')+b'\0';raw+=s+b'\0'*(len(s)%2)
 out+=j['name'].encode()+b'\0'+struct.pack('<HHI',j['pages'],len(j['labels']),len(raw))+raw
 for label in j['labels']:out+=struct.pack('<H',label['index'])+label['text'].encode('cp437')+b'\0'
(root/'tests/fixtures/presentation_programs.bin').write_bytes(out)
print(len(rows),'TTM frames and',len(programs),'original programs exported')
