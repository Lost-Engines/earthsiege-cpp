#!/usr/bin/env python3
"""Export original patrol/search decisions and group attack transitions."""
import json
from pathlib import Path
import struct
import sys
root = Path(__file__).resolve().parents[1]
analysis = Path(sys.argv[1])
r = json.loads((analysis / 'missions/engagement.json').read_text())
controllers = {v['address']:i for i,v in enumerate(json.loads((analysis / 'missions/controllers.json').read_text())['tables'])}
actors = {0:-1,0x1020000:0,0x1021000:1,0x1022000:2}
with (root / 'tests/fixtures/engagement.bin').open('wb') as out:
    def pack(fmt,*v): out.write(struct.pack('<' + fmt,*v))
    def state(s): pack('BiIBBB',controllers[s['controller']],s['clock'],s['ticks'],s['updated'],s['first'],s['changed'])
    out.write(b'ESENG001');pack('I',len(r['checks']))
    for c in r['checks']:
        mode = ['herc_search','herc_patrol','flyer_search','flyer_patrol'].index(c['kind'])
        pack('BBhh3BBhBhhhH',mode,c['leader'],c['command'],c['order'],*c['bad'],c['has_old'],actors[c['selection']],c['designated'],c['targetkind'],c['timer'],c['ratio'],c['stagger'])
        v=c['result']
        pack('hB2hhBhBiHIBB',actors[v['target']],v['changed'],*v['counts'],v['timer'],controllers[v['controller']],v['aim'],v['a6'],v['controller_clock'],v['stagger'],v['ticks'],v['first'],v['updated'])
        pack('B',len(v['events']))
        for kind,*args in v['events']:
            pack('BB',0 if kind.startswith('navigate') else 1 if kind=='select' else 2,args[0] if args else 0)
    pack('H',len(r['broadcast']))
    for c in r['broadcast']:
        pack('h',actors[c['target']])
        for dead,target in c['followers']:pack('Bh',dead,actors[target])
        pack('2h',*c['counts'])
        for s in c['states']:state(s)
print(len(r['checks']),'engagement decisions and',len(r['broadcast']),'group transitions imported')
