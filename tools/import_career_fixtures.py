#!/usr/bin/env python3
"""Import original GO save transactions and campaign branches, without predicting them."""
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);destination=Path(__file__).resolve().parents[1]/'tests/fixtures'
out=bytearray(b'ESCAR001')
def pack(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
cases=json.loads((source/'missions/career-fixtures.json').read_text())['cases'];pack('I',len(cases))
for case in cases:
 pack('H',case['history'])
 for name in ['input','saved','restored']:
  data=bytes.fromhex(case[name]);pack('I',len(data));out.extend(data)
(destination/'career.bin').write_bytes(out)
out=bytearray(b'ESCAM001')
cases=json.loads((source/'missions/campaign.json').read_text())['campaigns'];pack('I',len(cases['transitions']))
for table,node,step,branch,name,changed,newstep,newnode in cases['transitions']:
 for text in [table.split('/')[-1],name]:out.extend(text.encode()+b'\0')
 pack('6h',node,step,branch,changed,newstep,newnode)
pack('I',len(cases['thresholds']))
for threshold in cases['thresholds']:pack('h',threshold)
(destination/'campaign.bin').write_bytes(out)
