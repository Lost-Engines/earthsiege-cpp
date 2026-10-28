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
out=bytearray(b'ESWFRM01')
def write(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def random(v):write('56h2B',*v)
def words(v):write('H',len(v));write('h'*len(v),*v)
write('H',len(rows))
for row in rows:
 write('Hh',row['demo'],row['ratio']);random(row['initial_random'])
 write('HHH',len(row['steps']),len(row['steps'][0]['hercs']),len(row['steps'][0]['flyers']))
 for frame in row['steps']:
  for h in frame['hercs']:
   write('3i3H4h4hHiIi',*h['position'],*h['angles'],*h['motion'],*h['aim'],h['controller'],h['clock'],h['ticks'],h['timer'])
   write('hihB9Bh4hh',h['cursor'],h['recovery'],h['bias'],h['reverse'],*h['flags'],h['power'],*h['shields'],h['strength'])
   for name in ['internal','armour','present','cells']:words(h[name])
  for f in frame['flyers']:
   write('3i3H3h3hhhBHiB',*f['position'],*f['angles'],*f['velocity'],*f['rates'],f['strength'],f['timer'],f['obstacle'],f['controller'],f['clock'],f['detectable'])
  random(frame['random'])
a.output.write_bytes(out)
print(len(rows),'cases;',sum(len(r['steps'])for r in rows),'frames;',len(out),'bytes')
