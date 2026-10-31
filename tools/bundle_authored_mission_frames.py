#!/usr/bin/env python3
"""Bundle passing authored-world captures and their provenance for C++ regression.

The full captures remain in the analysis tree. --frames may retain only a prefix
for the checked-in regression fixture; no simulation frames are skipped inside
that prefix. Overrides/repairs from the original probe remain explicit.
"""
import argparse,hashlib,json,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--frames',type=int,default=64)
parser.add_argument('--report',type=Path,help='Native recheck report for the same saved original captures')
parser.add_argument('--case',action='append',help='Retain only this exact capture label; repeat to select several')
args=parser.parse_args()
if args.frames<1:parser.error("--frames must be positive")
report=json.loads((args.capture/'checks.json').read_text())
checks=json.loads(args.report.read_text()) if args.report else report
selected=[r for r in checks['cases'] if not args.case or r['label'] in args.case]
if args.case and set(args.case)-{r['label']for r in selected}:raise ValueError('unknown capture label')
if not selected or not all(r['passed']for r in selected):raise ValueError('selected captures contain failed comparisons')
rows=[];manifest=[]
for r in selected:
 path=args.capture/'cases'/(r['label'].replace('/','__')+'.json')
 case=json.loads(path.read_text())['cases'][0]
 count=len(case['steps']);case['steps']=case['steps'][:args.frames]
 rows.append(case)
 manifest.append(dict(label=case['label'],captured_frames=count,retained_frames=len(case['steps']),
                      capture_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                      handoff_sha256={k:hashlib.sha256(bytes.fromhex(v)).hexdigest()for k,v in case['handoff'].items()},
                      formation_repairs=case.get('formation_repairs',[]),
                      effect_queue_repairs=case.get('effect_queue_repairs',[]),
                      initial_aim_repairs=case.get('initial_aim_repairs',[]),
                      offset_overflow_repair=case.get('offset_overflow_repair')))
with tempfile.TemporaryDirectory(prefix='earthsiege-world-fixture-')as directory:
 output_json=Path(directory)/'capture.json'
 output_json.write_text(json.dumps(dict(cases=rows),separators=(',',':'))+'\n')
 subprocess.run([sys.executable,str(ROOT/'tools/export_mission_world_frame_fixtures.py'),str(output_json),str(args.output)],check=True)
args.output.with_suffix('.provenance.json').write_text(json.dumps(dict(
 original_sha256=report['original_sha256'],scope=report['scope'],ratio=report['ratio'],simulation_seed=report['simulation_seed'],
 player_input=report.get('player_input',False),launch_seeds=report.get('launch_seeds',[17]),
 fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),cases=manifest),indent=2)+'\n')
