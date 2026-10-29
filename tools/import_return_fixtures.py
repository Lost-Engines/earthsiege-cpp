#!/usr/bin/env python3
"""Pack complete original return/save captures; no reimplementation of the rules."""
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);data=json.loads((source/'missions/shell-return-fixtures.json').read_text());out=bytearray(b'ESRET001')
def pack(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
pack('I',len(data['cases']))
for c in data['cases']:
 for key in ['career','hangar','results','saved','hangar_after']:
  value=bytes.fromhex(c[key]);pack('I',len(value));out.extend(value)
 pack('100h',*c['counters']);pack('56H2B',*c['rng']);pack('56H2B',*c['rng_after']);pack('4h',c['dead'],*c['statistics'])
(Path(__file__).resolve().parents[1]/'tests/fixtures/campaign_return.bin').write_bytes(out)
