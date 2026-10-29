#!/usr/bin/env python3
"""Package coherent original GO launch captures without reconstructing their output."""
import json,struct,sys
from pathlib import Path
source=Path(sys.argv[1]);dest=Path(__file__).resolve().parents[1]/'tests/fixtures'
out=bytearray(b'ESNEW001')
def pack(fmt,*values):out.extend(struct.pack('<'+fmt,*values))
def blob(value):pack('I',len(value));out.extend(value)
def rng(r):pack('56H2B',*(v&65535 for v in r['words']),*r['indices'])
cases=json.loads((source/'missions/new-career.json').read_text())['cases'];pack('I',len(cases))
for c in cases:
 pack('3h',c['seed'],c['battle'],c['policy'])
 for k in ['runtime','saved','hangar']:blob(bytes.fromhex(c[k]))
 rng(c['rng'])
(dest/'new_career.bin').write_bytes(out)
rows=[]
for path in sorted((source/'missions/launches').glob('*/*.json')):
 d=json.loads(path.read_text());m=d.get('metadata',{})
 if 'rng_before_launch'in m:rows.append((path,m))
out=bytearray(b'ESLCH001');pack('I',len(rows))
for path,m in rows:
 out.extend(m['name'].encode()+b'\0');pack('4h',m['mode'],m['node']or 0,m['step']or 0,m['rng_seed']);pack('5h',*m['menu_options']);pack('100h',*m['history_before']);rng(m['rng_before_launch']);rng(m['rng_after_launch'])
 for name in ['script.dat','mission.var','player.mec']:blob((path.with_suffix('')/name).read_bytes())
(dest/'launch.bin').write_bytes(out);print(len(rows),'coherent original launcher cases imported')
out=bytearray(b'ESLRS001');rows=json.loads((source/'missions/launch-roster.json').read_text())['cases'];pack('I',len(rows))
for c in rows:
 pack('2h',c['mode'],c['identity'])
 for key in ['state','hangar','player','hangar_after']:blob(bytes.fromhex(c[key]))
 rng(c['rng_before']);rng(c['rng_after'])
(dest/'launch_roster.bin').write_bytes(out)
