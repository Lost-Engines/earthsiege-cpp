#!/usr/bin/env python3
"""Export original behavior tables, entry/selection and outer phase dispatch."""
import json
from pathlib import Path
import struct
import sys
root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])
r = json.loads((analysis / 'missions/controllers.json').read_text())
dispatch = json.loads((analysis / 'missions/controller-dispatch.json').read_text())['cases']
indices = {row['address']:i for i,row in enumerate(r['tables'])}
with (root / 'tests/fixtures/controllers.bin').open('wb') as out:
    def pack(fmt, *v): out.write(struct.pack('<' + fmt, *v))
    def state(s): pack('BiIBBBH', s['definition'], s['clock'], s['ticks'], s['updated'], s['first'], s['skip'], s['stagger'])
    out.write(b'ESCTRL01'); pack('B', len(r['tables']))
    for t in r['tables']:
        name = t['name'].encode(); pack('B',len(name)); out.write(name)
        pack('iHh',t['countdown'],sum(v<<i for i,v in enumerate(t['flags'])), -1 if t['status'] is None else t['status'])
    pack('H',len(r['entries']))
    for address,stagger,steps in r['entries']:
        pack('BH',indices[address],stagger)
        for ticks,flags in steps: pack('IBB', ticks,*flags)
    pack('H',len(r['selection']))
    for kind,*v in r['selection']:
        if kind == 'herc': pack('BBhhBB',0,*v[:-1],indices[v[-1]])
        else: pack('BhB',1,v[0],indices[v[1]])
    pack('H',len(r['statuses']))
    for address,disabled,dead,command,status in r['statuses']: pack('BBBhh',indices[address],disabled,dead,command,status)
    pack('H',len(dispatch))
    for c in dispatch:
        state(c);pack('hbbBB',c['ratio'],c['change_reconsider'],c['change_update'],c['skip_update'],c['move_result'])
        state(c['result']);pack('B',len(c['result']['events']))
        for phase,definition in c['result']['events']: pack('BB',['reconsider','update','movement'].index(phase),definition)
print(len(dispatch),'dispatches and',len(r['entries']),'state entries imported')
