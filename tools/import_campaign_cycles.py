#!/usr/bin/env python3
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((root.parent/'earthsiege-reverse/analysis/missions/campaign-cycles.json').read_text())
out=bytearray(b'ESCYC001'+struct.pack('<I',len(data['cases'])))
def blob(s):
 b=bytes.fromhex(s);out.extend(struct.pack('<I',len(b))+b)
def rng(s):out.extend(struct.pack('<56H2B',*(x&65535 for x in s['words']),*s['indices']))
for c in data['cases']:
 out+=struct.pack('<hhI',c['seed'],c['battle'],len(c['steps']))
 for t in c['steps']:
  out+=struct.pack('<h',t['return_seed'])
  for s in [t['before']['career'],t['before']['hangar'],t['results'],t['after']['career'],t['after']['hangar'],t['next_script'][:t['next_script_written']*2],t['next_player'],t['next_counters']]:blob(s)
  out+=struct.pack('<100h',*t['counters']);out+=t['next_name'].encode()+b'\0';rng(t['rng_return']);rng(t['rng_next'])
(root/'tests/fixtures/campaign_cycles.bin').write_bytes(out)
