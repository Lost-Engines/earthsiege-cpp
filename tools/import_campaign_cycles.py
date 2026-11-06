#!/usr/bin/env python3
import argparse,hashlib,json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description='Pack independent original campaign transaction captures.')
parser.add_argument('source',type=Path,nargs='?',default=root.parent/'earthsiege-reverse/work/research/missions/campaign-cycles.json')
parser.add_argument('output',type=Path,nargs='?',default=root/'tests/fixtures/campaign_cycles.bin')
args=parser.parse_args();data=json.loads(args.source.read_text())
journey='terminal'in data['cases'][0]
out=bytearray((b'ESCYC002'if journey else b'ESCYC001')+struct.pack('<I',len(data['cases'])))
def blob(s):
 b=bytes.fromhex(s);out.extend(struct.pack('<I',len(b))+b)
def rng(s):out.extend(struct.pack('<56H2B',*(x&65535 for x in s['words']),*s['indices']))
for c in data['cases']:
 out+=struct.pack('<hhI',c['seed'],c['battle'],len(c['steps']))
 for t in c['steps']:
  out+=struct.pack('<h',t['return_seed'])
  for s in [t['before']['career'],t['before']['hangar'],t['results'],t['after']['career'],t['after']['hangar'],t['next_script'][:t['next_script_written']*2],t['next_player'],t['next_counters']]:blob(s)
  out+=struct.pack('<100h',*t['counters']);out+=t['next_name'].encode()+b'\0';rng(t['rng_return']);rng(t['rng_next'])
  if journey:out+=struct.pack('<Bhh',t['terminal'],t['next_node'],t['next_step'])
args.output.write_bytes(out)
print(len(data['cases']),'campaigns,',sum(len(c['steps'])for c in data['cases']),'transactions,',len(out),'bytes')

if journey:
 original=root.parent/'earthsiege-reverse/extracted/cd_1994_10/GO.EXE'
 args.output.with_suffix('.provenance.json').write_text(json.dumps(dict(
  scope=data['scope'],capture_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
  original_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),
  fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
  cases=[dict(battle=c['battle'],failure=c['bad'],seed=c['seed'],transactions=len(c['steps']),
              terminal_node=c['steps'][-1]['next_node'])for c in data['cases']]),indent=2)+'\n')
