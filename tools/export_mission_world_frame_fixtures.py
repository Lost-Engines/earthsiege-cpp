#!/usr/bin/env python3
"""Pack independently captured original whole-mission navigation frames."""
import argparse
import json
from pathlib import Path
import struct
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path)
p.add_argument('output',type=Path)
a=p.parse_args()
rows=json.loads(a.capture.read_text())['cases']
player='player' in rows[0]['steps'][0]
interactive=player and 'target' in rows[0]['steps'][0]['input']
extended='outcome' in rows[0]['steps'][0]
out=bytearray(b'ESWFRM05' if interactive else b'ESWFRM04' if player else b'ESWFRM03' if extended else b'ESWFRM02')
def write(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def random(v):write('56h2B',*v)
def words(v):write('H',len(v));write('h'*len(v),*v)
write('HI',len(rows),sum(len(r['steps'])for r in rows))
for row in rows:
 write('HBh',row['demo'],row.get('friendly',True),row['ratio']);random(row['initial_random'])
 if interactive:write('h',row.get('missile_type',-1))
 write('HHH',len(row['steps']),len(row['steps'][0]['hercs']),len(row['steps'][0]['flyers']))
 for frame in row['steps']:
  if player:
   i=frame['input'];write('6h2i4BH',*i['movement'],*i['aiming'],*i['camera'],*i['mouse'],i['left'],i['right'],i['fire'],i['enter'],i['command'])
   if interactive:write('ihbBB',i['target'],i['selection'],i['tracking'],i['pair'],i['axis_source'])
  for h in frame['hercs']:
   write('3i3H4h4hHiIi',*h['position'],*h['angles'],*h['motion'],*h['aim'],h['controller'],h['clock'],h['ticks'],h['timer'])
   write('hihB9Bh4hh',h['cursor'],h['recovery'],h['bias'],h['reverse'],*h['flags'],h['power'],*h['shields'],h['strength'])
   for name in ['internal','armour','present','cells']:words(h[name])
  for f in frame['flyers']:
   write('3i3H3h3hhhBHiB',*f['position'],*f['angles'],*f['velocity'],*f['rates'],f['strength'],f['timer'],f['obstacle'],f['controller'],f['clock'],f['detectable'])
  random(frame['random'])
  if extended:write('12i',*frame['outcome'])
  if player:
   p=frame['player'];write('h5B2i5BB',p['selection'],p['group'],p['hold'],p['tracking'],p['manual_fired'],p['manual_active'],p['target'],p['displayed'],*p['locks'],p['locked'])
   write('H',len(p['slots']))
   for w in p['slots']:
    write('h',w['id']if w else -1)
    if w:write('hBBhh3h',w['refire'],w['disabled'],w['linked'],w['remaining'],w['setting'],*w['convergence'])
a.output.write_bytes(out)
print(len(rows),'cases;',sum(len(r['steps'])for r in rows),'frames;',len(out),'bytes')
