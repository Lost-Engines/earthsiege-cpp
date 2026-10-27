#!/usr/bin/env python3
"""Import original constructed weapon banks and allocation sequences."""
import json
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'definitions/weapon-banks.json').read_text())['cases']
out=bytearray(b'ESWBANK2')+struct.pack('<I',len(rows))
def shorts(values):
    global out
    out+=struct.pack('<H'+'h'*len(values),len(values),*values)
def state(values):
    global out
    out+=struct.pack('<H',len(values))
    keys=['weapon','slot','projectile','disabled','remaining','flags','refreshed','cell','cell_count','fast','firing','ready','priority','setting','charge','rounds','spinning','elf_ready']
    for value in values:
        out+=struct.pack('<B',value is not None)
        if value is not None:out+=struct.pack('<'+'i'*len(keys),*(value.get(k)if value.get(k)is not None else -1 for k in keys))
for row in rows:
    out+=row['name'].encode()+b'\0';shorts(row['equipment']);shorts(row['selection_values']);state(row['initial'])
    out+=struct.pack('<H',len(row['steps']))
    for step in row['steps']:
        out+=struct.pack('<8h',*(step[k]for k in ['ratio','budget','selected','free','changed','action','disabled','result']));state(step['state'])
    out+=struct.pack('<5hH',*row['power_initial'],len(row['reactor_steps']))
    for step in row['reactor_steps']:
        out+=struct.pack('<7h',step['ratio'],step['free'],*step['power']);state(step['state'])
(root/'tests/fixtures/weapon_banks.bin').write_bytes(out)
print(len(rows),'original weapon banks')
