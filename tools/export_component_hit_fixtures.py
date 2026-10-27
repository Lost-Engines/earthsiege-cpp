#!/usr/bin/env python3
"""Import original articulated component selection and assembled loadout volumes."""
import json, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=json.loads((Path(sys.argv[1])/'models/native-component-hits.json').read_text())
out=bytearray(b'ESCHIT01')+struct.pack('<H',len(rows))
def shorts(values):return struct.pack('<H'+'h'*len(values),len(values),*values)
for row in rows:
 out+=row['name'].encode()+b'\0'+shorts(row['equipment'])+struct.pack('<3i3H',*row['position'],*row['angles'])
 out+=struct.pack('<H',len(row['definition']))
 for part in row['definition']:
  out+=struct.pack('<hH',part['part'],len(part['sections']))
  for section in part['sections']:
   out+=struct.pack('<h4hH',section['section'],*section['bounds'],len(section['spheres']))
   for sphere in section['spheres']:out+=struct.pack('<4h',*sphere)
 out+=struct.pack('<H',len(row['steps']))
 for step in row['steps']:
  out+=struct.pack('<5hH',*step['controls'],step['ratio'],len(step['cases']))
  for case in step['cases']:
   out+=struct.pack('<3H3iih',*case['angles'],*case['translation'],case['reach'],case['radius'])+shorts(case['present'])
   out+=struct.pack('<BiH',case['result']is not None,*(case['result']or[0,0]))
(root/'tests/fixtures/component_hits.bin').write_bytes(out)
print(sum(len(step['cases'])for row in rows for step in row['steps']),'original component queries')
