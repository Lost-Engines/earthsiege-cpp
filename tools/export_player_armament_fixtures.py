#!/usr/bin/env python3
"""Import executable captures for player weapon selection and trigger policy."""
import json, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'definitions/player-armament.json').read_text())['cases']
out=bytearray(b'ESPARM01')+struct.pack('<I',len(rows))
def ints(values):
    out.extend(struct.pack('<H'+'i'*len(values),len(values),*values))
def state(value):
    out.extend(struct.pack('<6i',*(value[k]for k in ['selected','group','hold','tracking','manual_fired','manual_active'])))
    for group in value['groups']:ints(group)
    out.extend(struct.pack('<H',len(value['slots'])))
    keys=['weapon','disabled','linked','available','remaining','flags','refreshed','cell','firing','fast','setting','charge','rounds','spinning','elf_ready','elf_remaining','holding','throw_range']
    for slot in value['slots']:
        out.extend(struct.pack('<B',slot is not None))
        if slot is not None:out.extend(struct.pack('<18i',*(slot[k]for k in keys)))
for row in rows:
    out.extend(row['name'].encode()+b'\0');ints(row['equipment']);ints(row['values']);state(row['initial'])
    out.extend(struct.pack('<H',len(row['steps'])))
    for step in row['steps']:
        out.extend(struct.pack('<H',step['action']));ints(step['args'])
        out.extend(struct.pack('<iB',step['result'],step['released']));ints(step['fired']);state(step['state'])
(root/'tests/fixtures/player_armament.bin').write_bytes(out)
print(len(rows),'player weapon banks;',len(out),'fixture bytes')
