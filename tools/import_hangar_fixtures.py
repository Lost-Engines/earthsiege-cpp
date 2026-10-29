#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);out=bytearray(b'ESHGR001')
def pack(fmt,*args):out.extend(struct.pack('<'+fmt,*args))
data=json.loads((source/'missions/hangar-fixtures.json').read_text())
for model in data['checks']['component_prices']:pack('ih16h',model['price'],model['construction_missions'],*model['armor_costs'],*model['internal_costs'])
pack('18h',*data['checks']['weapon_prices'])
pack('I',len(data['repairs']))
for c in data['repairs']:
 pack('hH',c['model'],len(c['weapons']));pack('h'*len(c['weapons']),*c['weapons']);pack('33h5ii33hii',*c['conditions'],*c['costs'],c['budget'],*c['after'],c['total'],c['share'])
pack('I',len(data['construction']))
for c in data['construction']:
 pack('hhH',c['model'],c['remaining'],len(c['weapons']));pack('h'*len(c['weapons']),*c['weapons']);pack('33h33hhhi',*c['conditions'],*c['after'],c['slot'],c['remaining_after'],c['value'])
(Path(__file__).resolve().parents[1]/'tests/fixtures/hangar.bin').write_bytes(out)
