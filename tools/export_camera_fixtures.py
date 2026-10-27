#!/usr/bin/env python3
"""Import original camera states and the inverse-sine lookup."""
import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
report=json.loads((Path(sys.argv[1])/'interface/native-camera.json').read_text())
out=bytearray(b'ESCAM001')+struct.pack('<I',len(report['cases']))
def matrix(m):return struct.pack('<9hBB3i',*m)
def state(s):
 return struct.pack('<3i3H2h3H4h9h',*s['position'],*s['angles'],s['distance'],s['movement'],*s['orbit'],*s['rates'],s['mode'],*s['focus'],*s['attachment_offset'],*s['look'])
for row in report['cases']:
 target=row['target'];out+=struct.pack('<3H',*target['angles'])+matrix(target['matrix'])+bytes([target['attachment']is not None])
 if target['attachment']:out+=matrix(target['attachment'])
 out+=state(row['before'])+state(row['after'])
out+=struct.pack('<I',len(report['controls']))
for row in report['controls']:out+=state(row['before'])+struct.pack('<B2h',row['zoom'],row['tilt'],row['turn'])+state(row['after'])
(root/'tests/fixtures/camera.bin').write_bytes(out)
values=report['arcsine']
text='#pragma once\n#include <array>\n#include <cstdint>\n\nnamespace earthsiege {\n\n// original 0x5f54c positive Q14 inverse-sine samples after four-unit rounding\ninline std::array<uint16_t, '+str(len(values))+'> constexpr arcsine_table{\n'
text+=''.join('  '+', '.join(map(str,values[i:i+24]))+',\n'for i in range(0,len(values),24))
text+='};\n\n} // namespace earthsiege\n'
(root/'src/simulation/arcsine_table.h').write_text(text)
print(len(report['cases']),'original camera updates;',len(values),'inverse-sine samples')
