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
authored='handoff' in rows[0]
player='player' in rows[0]['steps'][0]
if any(('handoff' in r)!=authored or ('player' in r['steps'][0])!=player for r in rows):
 raise ValueError('world fixture cases must use the same authored/player-input format')
interactive=player and 'target' in rows[0]['steps'][0]['input']
if authored and player and not interactive:
 raise ValueError('authored player fixture requires target/weapon input fields')
extended='outcome' in rows[0]['steps'][0]
out=bytearray(b'ESWFRM07' if authored and player else b'ESWFRM06' if authored else b'ESWFRM05' if interactive else b'ESWFRM04' if player else b'ESWFRM03' if extended else b'ESWFRM02')
def write(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def random(v):write('56h2B',*v)
def words(v):write('H',len(v));write('h'*len(v),*v)
write('HI',len(rows),sum(len(r['steps'])for r in rows))
for row in rows:
 write('HBh',row['demo'],row.get('friendly',True),row['ratio']);random(row['initial_random'])
 if authored:
  out.extend(row['label'].encode()+b'\0')
  for name in ['script.dat','player.mec','mission.var']:
   data=bytes.fromhex(row['handoff'][name]);write('I',len(data));out.extend(data)
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
  if authored:
   m=frame['mission'];write('100hH',*m['counters'],m['stagger']);words(m['triggers'])
   write('H',len(m['groups']))
   for g in m['groups']:write('hB',*g)
   words(m['targets'])
   write('H',len(m['bases']))
   for b in m['bases']:
    write('3ihB',*b['position'],b['damage'],b['dead']);words(b['cells'])
  if player:
   p=frame['player'];write('h5B2i5BB',p['selection'],p['group'],p['hold'],p['tracking'],p['manual_fired'],p['manual_active'],p['target'],p['displayed'],*p['locks'],p['locked'])
   write('H',len(p['slots']))
   for w in p['slots']:
    write('h',w['id']if w else -1)
    if w:write('hBBhh3h',w['refire'],w['disabled'],w['linked'],w['remaining'],w['setting'],*w['convergence'])
a.output.write_bytes(out)
print(len(rows),'cases;',sum(len(r['steps'])for r in rows),'frames;',len(out),'bytes')
