#!/usr/bin/env python3
"""Import captured original mission output and hash its in-memory reference tables."""
from pathlib import Path
import argparse,json,shutil,struct,zlib
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('analysis',type=Path,help='earthsiege-reverse/analysis directory')
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
base=args.analysis/'missions/compiled';rows=[]
def checksum(raw):return f'{zlib.crc32(raw):08x}'
def shorts(values):return struct.pack('<'+'h'*len(values),*values)
for source in sorted(base.rglob('*.json')):
 native=json.loads(source.read_text());tables=native['tables'];payloads=[shorts(native['header'])]
 for name in ['points','headings','routes','areas','timers','hercs','flyers','orders','groups']:
  data=bytearray()
  for record in tables[name]:
   if record['words'][0]==-1:continue
   if name=='points':data.extend(struct.pack('<3i',*record['position']))
   elif name=='headings':data.extend(shorts(record['words'][4:]))
   elif name=='routes':data.extend(struct.pack('<H',len(record['points'])));data.extend(shorts(record['points']))
   else:data.extend(shorts(record['words'][3 if name in ['areas','timers','groups']else 4:]))
  payloads.append(data)
 relative=source.relative_to(base).with_suffix('.dat');data_file=source.with_suffix('.dat');destination=root/'tests/fixtures/scripts'/relative;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(data_file,destination)
 rows.append(f'{relative.as_posix()} {data_file.stat().st_size} '+' '.join(map(checksum,payloads))+'\n')
(root/'tests/script_checks.tsv').write_text(''.join(rows));print(len(rows),'original compiled mission fixtures imported')
