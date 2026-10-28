#!/usr/bin/env python3
"""Export original skimmer attack control and rocket launch speed traces."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];d=json.loads((Path(sys.argv[1])/'definitions/flyer-attack.json').read_text());out=bytearray(b'ESFATK01')
def pack(fmt,*v):out.extend(struct.pack('<'+fmt,*v))
def flight(s):pack('3i3H3h3hhhB',*s['position'],*s['angles'],*s['velocity'],*s['rates'],s['strength'],s['timer'],s['obstacle'])
def attack(s):pack('4hi2B',s['phase'],s['requested'],s['latch'],s['refire'],s['manoeuvre'],s['window'],s['contact'])
pack('H',len(d['cases']))
for r in d['cases']:
 flight(r['flight']);attack(r['attack']);pack('56h2B',*r['random']);pack('H',len(r['steps']))
 for s in r['steps']:
  t=s['target'];pack('3iHhh',*t['position'],t['heading'],t['speed'],s['ratio']);flight(s['flight']);attack(s['attack']);pack('56h2B',*s['random']);pack('H',len(s['shots']))
  for shot in s['shots']:pack('9i',*shot)
pack('H',len(d['launch_speeds']))
for row in d['launch_speeds']:pack('2h',*row)
(root/'tests/fixtures/flyer_attack.bin').write_bytes(out)
print(sum(len(r['steps'])for r in d['cases']),'flyer attack updates exported')
