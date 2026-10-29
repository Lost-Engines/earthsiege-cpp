#!/usr/bin/env python3
"""Import assembled original HDD captures and their immutable handoff inputs."""
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);data=json.loads((source/'cockpits/hdd.json').read_text());out=bytearray(b'ESHDD001')
def pack(fmt,*values):out.extend(struct.pack('<'+fmt,*values))
for name in ['SCRIPT.DAT','PLAYER.MEC','MISSION.VAR']:
 b=bytes.fromhex(data['inputs'][name]);pack('I',len(b));out.extend(b)
pack('I',len(data['rectangles']))
for rect in data['rectangles']:pack('4i',*rect)
pack('I',len(data['cases']))
for row in data['cases']:
 pack('H2i',row['key'],row['command'],row['wingmate'])
 pixels=(source/'cockpits/hdd-frames'/row['frame']).read_bytes()
 for x0,y0,x1,y1 in data['rectangles']:
  for y in range(y0,y1+1):out.extend(pixels[y*320+x0:y*320+x1+1])
(Path(__file__).resolve().parents[1]/'tests/fixtures/cockpit_hdd.bin').write_bytes(out)
